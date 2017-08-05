
#include "db_thread.h"
#include "db_read.h"
#include "db_write.h"
#include "bit.h"
#include "ringbuf.h"
#include "util_alloc.h"
#include "util_atomic.h"
#include "util_clock.h"
#include "util_thread.h"
#include "zpacket.h"
#include "enum_zop.h"
#include "enum2str.h"

#define DB_THREAD_LOG_PATH "log/db_thread.txt"

typedef struct {
    int         dbId;
    uint32_t    writeCount;
    uint32_t    readCount;
    sqlite3*    db;
    ZPacket*    writes;
    ZPacket*    reads;
} DbInst;

struct DbThread {
    uint32_t    instCount;
    atomic32_t  nextInstId;
    int         cachedInstId;
    int*        instIds;
    DbInst*     insts;
    DbInst*     cachedInst;
    RingBuf*    queryQueue;
    RingBuf*    logQueue;
    int         logId;
};

static void db_thread_reply_error_only(DbThread* db, ZPacket* zpacket)
{
    ZPacket reply;
    
    memset(&reply, 0, sizeof(reply));
    reply.db.zResult.queryId = zpacket->db.zQuery.queryId;
    reply.db.zResult.hadError = true;
    reply.db.zResult.hadErrorUnprocessed = true;
    
    db_reply(db, zpacket, &reply);
}

static int db_thread_exec(DbThread* db, sqlite3* sqlite, const char* sql)
{
    char* errmsg = NULL;
    int rc = ERR_None;
    
    sqlite3_exec(sqlite, sql, NULL, NULL, &errmsg);
    
    if (errmsg)
    {
        log_writef(db->logQueue, db->logId, "ERROR: db_thread_exec: sqlite3_exec failed for SQL: \"%s\" with error message: \"%s\"", sql, errmsg);
        sqlite3_free(errmsg);
        rc = ERR_Library;
    }
    
    return rc;
}

static void db_thread_execute_writes(DbThread* db, DbInst* inst, uint32_t count)
{
    uint64_t timer = clock_microseconds();
    sqlite3* sqlite = inst->db;
    ZPacket* writes = inst->writes;
    uint32_t i;

    /* If we have multiple write queries to execute, we batch them all into a single transaction, to be more efficient */
    db_thread_exec(db, sqlite, "BEGIN");
    
    for (i = 0; i < count; i++)
    {
        dbw_dispatch(db, sqlite, &writes[i]);
    }
    
    db_thread_exec(db, sqlite, "COMMIT");
    
    timer = clock_microseconds() - timer;
    log_writef(db->logQueue, db->logId, "Executed TRANSACTION containing %u writes in %llu microseconds", count, timer);
    
    inst->writeCount = 0;
}

static void db_thread_execute_reads(DbThread* db, DbInst* inst, uint32_t count)
{
    sqlite3* sqlite = inst->db;
    ZPacket* reads = inst->reads;
    uint32_t i;

    for (i = 0; i < count; i++)
    {
        dbr_dispatch(db, sqlite, &reads[i]);
    }
    
    inst->readCount = 0;
}

static void db_thread_execute_queries(DbThread* db)
{
    uint32_t count = db->instCount;
    DbInst* insts = db->insts;
    uint32_t i;

    for (i = 0; i < count; i++)
    {
        DbInst* inst = &insts[i];
        uint32_t writeCount = inst->writeCount;
        uint32_t readCount = inst->readCount;

        if (writeCount)
            db_thread_execute_writes(db, inst, writeCount);

        if (readCount)
            db_thread_execute_reads(db, inst, readCount);
    }
}

static DbInst* db_thread_get_inst(DbThread* db, int dbId)
{
    DbInst* insts;
    uint32_t n, i;

    if (db->cachedInstId == dbId)
        return db->cachedInst;

    insts = db->insts;
    n = db->instCount;

    for (i = 0; i < n; i++)
    {
        DbInst* inst = &insts[i];

        if (inst->dbId == dbId)
        {
            db->cachedInstId = dbId;
            db->cachedInst = inst;
            return inst;
        }
    }

    return NULL;
}

static void db_thread_destruct_zpacket(DbThread* db, ZPacket* zpacket)
{
    int zop = zpacket->db.zQuery.zop;
    
    switch (zop)
    {
    case ZOP_DB_QueryMainGuildList:
    case ZOP_DB_QueryLoginCredentials:
    case ZOP_DB_QueryCSCharacterInfo:
    case ZOP_DB_QueryCSCharacterNameAvailable:
    case ZOP_DB_QueryMainLoadCharacter:
        dbr_destruct(db, zpacket, zop);
        break;
    
    case ZOP_DB_QueryLoginNewAccount:
    case ZOP_DB_QueryCSCharacterCreate:
    case ZOP_DB_QueryCSCharacterDelete:
        dbw_destruct(db, zpacket, zop);
        break;
    
    default:
        log_writef(db->logQueue, db->logId, "WARNING: db_thread_destruct_zpacket: received unexpected zop: %s", enum2str_zop(zop));
        break;
    }
}

static void db_thread_queue_query(DbThread* db, ZPacket* zpacket, int zop, int isWrite)
{
    DbInst* inst = db_thread_get_inst(db, zpacket->db.zQuery.dbId);
    ZPacket* array;
    uint32_t index;

    if (!inst) goto error;

    zpacket->db.zQuery.zop = zop;

    if (isWrite)
    {
        array = inst->writes;
        index = inst->writeCount;
    }
    else
    {
        array = inst->reads;
        index = inst->readCount;
    }

    if (bit_is_pow2_or_zero(index))
    {
        uint32_t cap = (index == 0) ? 1 : index * 2;
        ZPacket* newArray = realloc_array_type(array, cap, ZPacket);

        if (!newArray) goto error;

        array = newArray;

        if (isWrite)
            inst->writes = array;
        else
            inst->reads = array;
    }

    array[index] = *zpacket;
    index++;

    if (isWrite)
        inst->writeCount = index;
    else
        inst->readCount = index;
    
    return;
    
error:
    /* Inform the sender of this query that it could not be processed */
    db_thread_reply_error_only(db, zpacket);
    db_thread_destruct_zpacket(db, zpacket);
}

static bool db_thread_create_database(DbThread* db, DbInst* inst, ZPacket* zpacket)
{
    StaticBuffer* schema = sbuf_create_from_file(sbuf_str(zpacket->db.zOpen.schemaPath));
    int rc;
    
    sbuf_grab(schema);
    
    if (!schema)
    {
        log_writef(db->logQueue, db->logId, "ERROR: db_thread_open_database: failed to open schema \"%s\"", sbuf_str(zpacket->db.zOpen.schemaPath));
        goto error;
    }

    rc = sqlite3_open_v2(
        sbuf_str(zpacket->db.zOpen.path),
        &inst->db,
        SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX, /*fixme: make mutex/threadsafety level selectable*/
        NULL);
    
    if (rc != SQLITE_OK)
    {
        log_writef(db->logQueue, db->logId, "ERROR: db_thread_open_database: failed to create database file \"%s\", reason: \"%s\"",
            sbuf_str(zpacket->db.zOpen.path), sqlite3_errstr(rc));
        goto error;
    }
    
    rc = db_thread_exec(db, inst->db, sbuf_str(schema));
    if (rc)
    {
        log_writef(db->logQueue, db->logId, "ERROR: db_thread_open_database: failed to write schema from \"%s\"", sbuf_str(zpacket->db.zOpen.schemaPath));
        goto error_open;
    }
    
    log_writef(db->logQueue, db->logId, "Created database file \"%s\" with schema \"%s\"", sbuf_str(zpacket->db.zOpen.path), sbuf_str(zpacket->db.zOpen.schemaPath));
    sbuf_drop(schema);
    return true;
    
error_open:
    sqlite3_close_v2(inst->db);
    inst->db = NULL;
error:
    sbuf_drop(schema);
    return false;
}

static void db_thread_open_database(DbThread* db, ZPacket* zpacket)
{
    DbInst* inst;
    uint32_t index = db->instCount;
    ZPacket errPacket;
    int dbId = zpacket->db.zOpen.dbId;
    int rc;
    
    if (bit_is_pow2_or_zero(index))
    {
        uint32_t cap = (index == 0) ? 1 : index * 2;
        DbInst* insts = realloc_array_type(db->insts, cap, DbInst);
        
        if (!insts) goto error;
        
        db->insts = insts;
        db->cachedInstId = -1;
        db->cachedInst = NULL;
    }
    
    inst = &db->insts[index];
    db->instCount = index + 1;
    
    inst->dbId = dbId;
    inst->writeCount = 0;
    inst->readCount = 0;
    inst->db = NULL;
    inst->writes = NULL;
    inst->reads = NULL;
    
    rc = sqlite3_open_v2(
        sbuf_str(zpacket->db.zOpen.path),
        &inst->db,
        SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX, /*fixme: make mutex/threadsafety level selectable*/
        NULL);
    
    if (rc != SQLITE_OK)
    {
        if (rc != SQLITE_CANTOPEN)
        {
            log_writef(db->logQueue, db->logId, "ERROR: db_thread_open_database: failed to open database file \"%s\", reason: \"%s\"",
                sbuf_str(zpacket->db.zOpen.path), sqlite3_errstr(rc));
            goto error;
        }
        
        if (!db_thread_create_database(db, inst, zpacket))
        {
            goto error;
        }
    }
    else
    {
        log_writef(db->logQueue, db->logId, "Opened database file \"%s\"", sbuf_str(zpacket->db.zOpen.path));
        
        /* Update statistics to improve query performance */
        db_thread_exec(db, inst->db, "ANALYZE");
    }
    
    goto finish;
    
error:
    errPacket.db.zOpen.dbId = dbId;
    rc = ringbuf_push(zpacket->db.zOpen.errorQueue, ZOP_DB_OpenDatabase, &errPacket);
    
    if (rc)
    {
        log_writef(db->logQueue, db->logId, "ERROR: db_thread_open_database: ringbuf_push() failed: %s", enum2str_err(rc));
    }
    
finish:
    zpacket->db.zOpen.path = sbuf_drop(zpacket->db.zOpen.path);
    zpacket->db.zOpen.schemaPath = sbuf_drop(zpacket->db.zOpen.schemaPath);
}

static ZPacket* db_thread_free_zpacket_array(DbThread* db, ZPacket* array, uint32_t count)
{
    if (array)
    {
        uint32_t i;
        
        for (i = 0; i < count; i++)
        {
            db_thread_destruct_zpacket(db, &array[i]);
        }
        
        free(array);
    }
    
    return NULL;
}

static void db_thread_free_inst(DbThread* db, DbInst* inst)
{
    sqlite3_close_v2(inst->db);
    inst->dbId = 0;
    inst->db = NULL;
    inst->writes = db_thread_free_zpacket_array(db, inst->writes, inst->writeCount);
    inst->reads = db_thread_free_zpacket_array(db, inst->reads, inst->readCount);
    inst->writeCount = 0;
    inst->readCount = 0;
}

static void db_thread_close_database(DbThread* db, ZPacket* zpacket)
{
    int dbId = zpacket->db.zOpen.dbId;
    DbInst* insts = db->insts;
    uint32_t n = db->instCount;
    uint32_t i;
    
    for (i = 0; i < n; i++)
    {
        DbInst* inst = &insts[i];
        
        if (inst->dbId == dbId)
        {
            db_thread_free_inst(db, inst);
            
            /* Swap and pop */
            n--;
            insts[i] = insts[n];
            db->instCount = n;
            break;
        }
    }
}

static void db_thread_proc(void* ptr)
{
    DbThread* db = (DbThread*)ptr;
    ZPacket zpacket;
    bool terminate = false;
    int count;
    int zop;
    int rc;

    for (;;)
    {
        /* The db thread is purely packet-driven, always blocks until something needs to be done */
        rc = ringbuf_wait(db->queryQueue);

        if (rc)
        {
            break;
        }

        /* Read as many input packets as possible */
        count = 0;

        for (;;)
        {
            rc = ringbuf_pop(db->queryQueue, &zop, &zpacket);
            if (rc) break;

            count++;

            switch (zop)
            {
            case ZOP_DB_TerminateThread:
                terminate = true;
                break;
            
            case ZOP_DB_OpenDatabase:
                db_thread_open_database(db, &zpacket);
                break;
            
            case ZOP_DB_CloseDatabase:
                db_thread_close_database(db, &zpacket);
                break;

            case ZOP_DB_QueryMainGuildList:
            case ZOP_DB_QueryLoginCredentials:
            case ZOP_DB_QueryCSCharacterInfo:
            case ZOP_DB_QueryCSCharacterNameAvailable:
            case ZOP_DB_QueryMainLoadCharacter:
                db_thread_queue_query(db, &zpacket, zop, false);
                break;

            case ZOP_DB_QueryLoginNewAccount:
            case ZOP_DB_QueryCSCharacterCreate:
            case ZOP_DB_QueryCSCharacterDelete:
                db_thread_queue_query(db, &zpacket, zop, true);
                break;

            default:
                break;
            }
        }

        /* Now process any queries we received */
        if (count)
            db_thread_execute_queries(db);

        if (terminate) break;
    }
}

DbThread* db_create(LogThread* log)
{
    DbThread* db = alloc_type(DbThread);
    int rc;

    if (!db) goto fail_alloc;

    db->instCount = 0;
    atomic32_set(&db->nextInstId, 1);
    db->instIds = NULL;
    db->insts = NULL;
    db->queryQueue = NULL;
    db->logQueue = log_get_queue(log);

    rc = log_open_file_literal(log, &db->logId, DB_THREAD_LOG_PATH);
    if (rc) goto fail;

    db->queryQueue = ringbuf_create_type(ZPacket, 1024);
    if (!db->queryQueue) goto fail;

    rc = thread_start(db_thread_proc, db);
    if (rc) goto fail;

    return db;

fail:
    db_destroy(db);
fail_alloc:
    return NULL;
}

static void db_free_insts(DbThread* db)
{
    DbInst* insts = db->insts;
    uint32_t count = db->instCount;
    
    if (insts)
    {
        uint32_t i;
        
        for (i = 0; i < count; i++)
        {
            db_thread_free_inst(db, &insts[i]);
        }
        
        free(insts);
        db->insts = NULL;
        db->instCount = 0;
    }
}

DbThread* db_destroy(DbThread* db)
{
    if (db)
    {
        db_free_insts(db);
        db->queryQueue = ringbuf_destroy(db->queryQueue);

        log_close_file(db->logQueue, db->logId);
        free(db);
    }

    return NULL;
}

int db_queue_query(RingBuf* dbQueue, RingBuf* replyQueue, int dbId, int queryId, int zop, ZPacket* zpacket)
{
    assert(zpacket != NULL);

    zpacket->db.zQuery.dbId = dbId;
    zpacket->db.zQuery.queryId = queryId;
    zpacket->db.zQuery.zop = zop;
    zpacket->db.zQuery.replyQueue = replyQueue;

    return ringbuf_push(dbQueue, zop, zpacket);
}

RingBuf* db_get_queue(DbThread* db)
{
    return db->queryQueue;
}

RingBuf* db_get_log_queue(DbThread* db)
{
    return db->logQueue;
}

int db_get_log_id(DbThread* db)
{
    return db->logId;
}

void db_reply(DbThread* db, ZPacket* zpacket, ZPacket* reply)
{
    int rc = ringbuf_push(zpacket->db.zQuery.replyQueue, zpacket->db.zQuery.zop, reply);
    
    if (rc)
    {
        log_writef(db->logQueue, db->logId, "ERROR: db_reply: ringbuf_push() failed: %s", enum2str_err(rc));
    }
}

int db_open(DbThread* db, RingBuf* errorQueue, int* outDbId, const char* path, int pathlen, const char* schemaPath, int schemalen)
{
    ZPacket zpacket;
    int dbId = atomic32_add(&db->nextInstId, 1);
    int rc;
    
    if (outDbId)
        *outDbId = dbId;
    
    zpacket.db.zOpen.dbId = dbId;
    zpacket.db.zOpen.errorQueue = errorQueue;
    zpacket.db.zOpen.path = sbuf_create(path, pathlen);
    zpacket.db.zOpen.schemaPath = sbuf_create(schemaPath, schemalen);
    
    sbuf_grab(zpacket.db.zOpen.path);
    sbuf_grab(zpacket.db.zOpen.schemaPath);
    
    if (!zpacket.db.zOpen.path || !zpacket.db.zOpen.schemaPath)
    {
        rc = ERR_OutOfMemory;
        goto error;
    }
    
    rc = ringbuf_push(db->queryQueue, ZOP_DB_OpenDatabase, &zpacket);
    if (rc) goto error;
    
    return ERR_None;
    
error:
    zpacket.db.zOpen.path = sbuf_drop(zpacket.db.zOpen.path);
    zpacket.db.zOpen.schemaPath = sbuf_drop(zpacket.db.zOpen.schemaPath);
    return rc;
}

int db_close(RingBuf* dbQueue, int dbId)
{
    ZPacket zpacket;
    zpacket.db.zOpen.dbId = dbId;
    return ringbuf_push(dbQueue, ZOP_DB_CloseDatabase, &zpacket);
}

bool db_prepare(DbThread* db, sqlite3* sqlite, sqlite3_stmt** stmt, const char* sql, int len)
{
    uint64_t timer = clock_microseconds();
    int rc = sqlite3_prepare_v2(sqlite, sql, len, stmt, NULL);
    
    if (rc == SQLITE_OK)
    {
        timer = clock_microseconds() - timer;
        log_writef(db->logQueue, db->logId, "Prepared query in %llu microseconds, SQL: \"%s\"", timer, sql);
        return true;
    }
    
    log_writef(db->logQueue, db->logId, "ERROR: db_prepare: sqlite3_prepare_v2() failed, reason: \"%s\", SQL: \"%s\"",
        sqlite3_errstr(rc), sql);
    return false;
}

int db_read(DbThread* db, sqlite3_stmt* stmt)
{
    for (;;)
    {
        int rc = sqlite3_step(stmt);
        
        switch (rc)
        {
        case SQLITE_BUSY:
        case SQLITE_LOCKED:
            continue;
        
        case SQLITE_ROW:
        case SQLITE_DONE:
            return rc;
        
        default:
            log_writef(db->logQueue, db->logId, "ERROR: db_read: sqlite3_step() failed, reason: \"%s\"", sqlite3_errstr(rc));
            return SQLITE_ERROR;
        }
    }
}

bool db_write(DbThread* db, sqlite3_stmt* stmt)
{
    for (;;)
    {
        int rc = sqlite3_step(stmt);
        
        switch (rc)
        {
        case SQLITE_BUSY:
        case SQLITE_LOCKED:
            continue;
        
        case SQLITE_DONE:
            return true;
        
        case SQLITE_ROW:
            log_write_literal(db->logQueue, db->logId, "WARNING: db_write: sqlite3_step() returned SQLITE_ROW, indicating data was SELECTed; is this a misconfigured READ query?");
            goto error;
        
        default:
            log_writef(db->logQueue, db->logId, "ERROR: db_write: sqlite3_step() failed, reason: \"%s\"", sqlite3_errstr(rc));
            goto error;
        }
    }
error:
    return false;
}

bool db_bind_int(DbThread* db, sqlite3_stmt* stmt, int col, int val)
{
    bool ret = true;
    int rc = sqlite3_bind_int(stmt, col + 1, val);
    
    if (rc != SQLITE_OK)
    {
        ret = false;
        log_writef(db->logQueue, db->logId, "ERROR: db_bind_int: sqlite3_bind_int() failed, reason: \"%s\"", sqlite3_errstr(rc)); 
    }
    
    return ret;
}

bool db_bind_int64(DbThread* db, sqlite3_stmt* stmt, int col, int64_t val)
{
    bool ret = true;
    int rc = sqlite3_bind_int64(stmt, col + 1, val);
    
    if (rc != SQLITE_OK)
    {
        ret = false;
        log_writef(db->logQueue, db->logId, "ERROR: db_bind_int64: sqlite3_bind_int64() failed, reason: \"%s\"", sqlite3_errstr(rc)); 
    }
    
    return ret;
}

bool db_bind_double(DbThread* db, sqlite3_stmt* stmt, int col, double val)
{
    bool ret = true;
    int rc = sqlite3_bind_double(stmt, col + 1, val);
    
    if (rc != SQLITE_OK)
    {
        ret = false;
        log_writef(db->logQueue, db->logId, "ERROR: db_bind_double: sqlite3_bind_double() failed, reason: \"%s\"", sqlite3_errstr(rc)); 
    }
    
    return ret;
}

bool db_bind_string(DbThread* db, sqlite3_stmt* stmt, int col, const char* str, int len)
{
    bool ret = true;
    int rc = sqlite3_bind_text(stmt, col + 1, str, len, SQLITE_STATIC);
    
    if (rc != SQLITE_OK)
    {
        ret = false;
        log_writef(db->logQueue, db->logId, "ERROR: db_bind_text: sqlite3_bind_text() failed, reason: \"%s\"", sqlite3_errstr(rc)); 
    }
    
    return ret;
}

bool db_bind_blob(DbThread* db, sqlite3_stmt* stmt, int col, const void* data, int len)
{
    bool ret = true;
    int rc = sqlite3_bind_blob(stmt, col + 1, data, len, SQLITE_STATIC);
    
    if (rc != SQLITE_OK)
    {
        ret = false;
        log_writef(db->logQueue, db->logId, "ERROR: db_bind_blob: sqlite3_bind_blob() failed, reason: \"%s\"", sqlite3_errstr(rc)); 
    }
    
    return ret;
}

bool db_bind_null(DbThread* db, sqlite3_stmt* stmt, int col)
{
    bool ret = true;
    int rc = sqlite3_bind_null(stmt, col + 1);
    
    if (rc != SQLITE_OK)
    {
        ret = false;
        log_writef(db->logQueue, db->logId, "ERROR: db_bind_null: sqlite3_bind_null() failed, reason: \"%s\"", sqlite3_errstr(rc));
    }
    
    return ret;
}

int db_fetch_int(sqlite3_stmt* stmt, int col)
{
    return sqlite3_column_int(stmt, col);
}

int64_t db_fetch_int64(sqlite3_stmt* stmt, int col)
{
    return sqlite3_column_int64(stmt, col);
}

double db_fetch_double(sqlite3_stmt* stmt, int col)
{
    return sqlite3_column_double(stmt, col);
}

const char* db_fetch_string(sqlite3_stmt* stmt, int col, int* outLength)
{
    const char* ptr = (const char*)sqlite3_column_text(stmt, col);
    
    if (outLength)
        *outLength = sqlite3_column_bytes(stmt, col);
    
    return ptr;
}

const byte* db_fetch_blob(sqlite3_stmt* stmt, int col, int* outLength)
{
    const byte* ptr = (const byte*)sqlite3_column_blob(stmt, col);
    
    if (outLength)
        *outLength = sqlite3_column_bytes(stmt, col);
    
    return ptr;
}

StaticBuffer* db_fetch_string_copy(sqlite3_stmt* stmt, int col)
{
    int len;
    const char* str = db_fetch_string(stmt, col, &len);
    
    if (!str || len < 0)
        return NULL;
    
    return sbuf_create(str, (uint32_t)len);
}

StaticBuffer* db_fetch_blob_copy(sqlite3_stmt* stmt, int col)
{
    int len;
    const byte* ptr = db_fetch_blob(stmt, col, &len);
    
    if (!ptr || len < 0)
        return NULL;
    
    return sbuf_create(ptr, (uint32_t)len);
}

int db_column_type(sqlite3_stmt* stmt, int col)
{
    return sqlite3_column_type(stmt, col);
}

bool db_column_is_null(sqlite3_stmt* stmt, int col)
{
    return (sqlite3_column_type(stmt, col) == SQLITE_NULL);
}
