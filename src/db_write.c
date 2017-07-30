
#include "db_write.h"
#include "login_crypto.h"
#include "util_clock.h"
#include "enum_zop.h"
#include "enum2str.h"

static void dbw_login_new_account(DbThread* db, sqlite3* sqlite, ZPacket* zpacket)
{
    sqlite3_stmt* stmt = NULL;
    ZPacket reply;
    bool run;
    
    reply.db.zResult.queryId = zpacket->db.zQuery.queryId;
    reply.db.zResult.hadError = true;
    reply.db.zResult.hadErrorUnprocessed = false;
    reply.db.zResult.rLoginNewAccount.client = zpacket->db.zQuery.qLoginNewAccount.client;
    reply.db.zResult.rLoginNewAccount.acctId = -1;
    
    run = db_prepare_literal(db, sqlite, &stmt,
        "INSERT INTO account "
        "   (name, password_hash, salt, creation_time)"
        "VALUES "
        "   (?, ?, ?, datetime('now'))");
    
    if (run)
    {
        StaticBuffer* acctName = zpacket->db.zQuery.qLoginNewAccount.accountName;
        
        if (db_bind_string_sbuf(db, sqlite, 0, acctName) &&
            db_bind_blob(db, sqlite, 1, zpacket->db.zQuery.qLoginNewAccount.passwordHash, LOGIN_CRYPTO_HASH_SIZE) &&
            db_bind_blob(db, sqlite, 2, zpacket->db.zQuery.qLoginNewAccount.salt, LOGIN_CRYPTO_SALT_SIZE))
        {
            int rc = db_write(db, stmt);
            
            switch (rc)
            {
            case SQLITE_DONE:
                reply.db.zResult.hadError = false;
                reply.db.zResult.rLoginNewAccount.acctId = sqlite3_last_insert_rowid(sqlite);
                break;
            
            case SQLITE_ERROR:
                break;
            }
        }
    }
    
    db_reply(db, zpacket, &reply);
    sqlite3_finalize(stmt);
}

void dbw_dispatch(DbThread* db, sqlite3* sqlite, ZPacket* zpacket)
{
    uint64_t timer = clock_microseconds();
    int zop = zpacket->db.zQuery.zop;
    
    switch (zop)
    {
    case ZOP_DB_QueryLoginNewAccount:
        dbw_login_new_account(db, sqlite, zpacket);
        break;
    
    default:
        log_writef(db_get_log_queue(db), db_get_log_id(db), "WARNING: dbw_dispatch: received unexpected zop: %s", enum2str_zop(zop));
        goto destruct;
    }
    
    timer = clock_microseconds() - timer;
    log_writef(db_get_log_queue(db), db_get_log_id(db), "Executed WRITE query %s in %llu microseconds", enum2str_zop(zop), timer);
destruct:
    dbw_destruct(db, zpacket, zop);
}

void dbw_destruct(DbThread* db, ZPacket* zpacket, int zop)
{
    switch (zop)
    {
    case ZOP_DB_QueryLoginNewAccount:
        zpacket->db.zQuery.qLoginNewAccount.accountName = sbuf_drop(zpacket->db.zQuery.qLoginNewAccount.accountName);
        break;
    
    default:
        log_writef(db_get_log_queue(db), db_get_log_id(db), "WARNING: dbw_destruct: received unexpected zop: %s", enum2str_zop(zop));
        break;
    }
}
