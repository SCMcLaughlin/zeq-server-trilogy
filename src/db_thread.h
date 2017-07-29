
#ifndef DB_THREAD_H
#define DB_THREAD_H

#include "define.h"
#include "buffer.h"
#include "log_thread.h"
#include "ringbuf.h"
#include "zpacket.h"
#include <sqlite3.h>

#define DB_THREAD_DB_PATH_DEFAULT "db/zeq.db"
#define DB_THREAD_DB_SCHEMA_PATH_DEFAULT "db/zeq_schema.sql"

typedef struct DbThread DbThread;

DbThread* db_create(LogThread* log);
DbThread* db_destroy(DbThread* db);

int db_queue_query(RingBuf* dbQueue, int zop, ZPacket* zpacket);

RingBuf* db_get_queue(DbThread* db);
RingBuf* db_get_log_queue(DbThread* db);
int db_get_log_id(DbThread* db);

void db_reply(DbThread* db, ZPacket* zpacket, ZPacket* reply);

int db_open(DbThread* db, RingBuf* errorQueue, int* outDbId, const char* path, int pathlen, const char* schemaPath, int schemalen);
#define db_open_literal(db, queue, outId, path, schema) db_open((db), (queue), (outId), path, sizeof(path) - 1, schema, sizeof(schema) - 1)
int db_close(RingBuf* dbQueue, int dbId);

bool db_prepare(DbThread* db, sqlite3* sqlite, sqlite3_stmt** stmt, const char* sql, int len);
#define db_prepare_literal(db, sqlite, stmt, sql) db_prepare((db), (sqlite), (stmt), sql, sizeof(sql) - 1)
int db_read(DbThread* db, sqlite3_stmt* stmt);

bool db_bind_int(DbThread* db, sqlite3_stmt* stmt, int col, int val);
bool db_bind_int64(DbThread* db, sqlite3_stmt* stmt, int col, int64_t val);
bool db_bind_double(DbThread* db, sqlite3_stmt* stmt, int col, double val);
bool db_bind_string(DbThread* db, sqlite3_stmt* stmt, int col, const char* str, int len);
#define db_bind_literal(db, stmt, col, str) db_bind_string((db), (stmt), (col), str, sizeof(str) - 1)
#define db_bind_string_sbuf(db, stmt, col, sbuf) db_bind_string((db), (stmt), (col), sbuf_str((sbuf)), sbuf_length((sbuf)))
bool db_bind_blob(DbThread* db, sqlite3_stmt* stmt, int col, const void* data, int len);
#define db_bind_blob_sbuf(db, stmt, col, sbuf) db_bind_blob((db), (stmt), (col), sbuf_data((sbuf)), sbuf_length((sbuf)))
bool db_bind_null(DbThread* db, sqlite3_stmt* stmt, int col);

int db_fetch_int(sqlite3_stmt* stmt, int col);
int64_t db_fetch_int64(sqlite3_stmt* stmt, int col);
double db_fetch_double(sqlite3_stmt* stmt, int col);
const char* db_fetch_string(sqlite3_stmt* stmt, int col, int* outLength);
const byte* db_fetch_blob(sqlite3_stmt* stmt, int col, int* outLength);
StaticBuffer* db_fetch_string_copy(sqlite3_stmt* stmt, int col);
StaticBuffer* db_fetch_blob_copy(sqlite3_stmt* stmt, int col);

int db_column_type(sqlite3_stmt* stmt, int col);
bool db_column_is_null(sqlite3_stmt* stmt, int col);

#endif/*DB_THREAD_H*/
