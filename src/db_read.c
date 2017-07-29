
#include "db_read.h"
#include "util_clock.h"
#include "enum_zop.h"
#include "enum2str.h"

static void dbr_login_credentials(DbThread* db, sqlite3* sqlite, ZPacket* zpacket)
{
    sqlite3_stmt* stmt = NULL;
    ZPacket reply;
    bool run;
    
    reply.db.zResult.queryId = zpacket->db.zQuery.queryId;
    reply.db.zResult.hadError = true;
    reply.db.zResult.rLoginCredentials.client = zpacket->db.zQuery.qLoginCredentials.client;
    reply.db.zResult.rLoginCredentials.loginId = -1;
    
    run = db_prepare_literal(db, sqlite, &stmt,
        "SELECT id, password_hash, salt FROM account WHERE name = ? LIMIT 1");
    
    if (run)
    {
        StaticBuffer* acctName = zpacket->db.zQuery.qLoginCredentials.accountName;
        
        if (db_bind_string_sbuf(db, stmt, 0, acctName))
        {
            const byte* passhash;
            const byte* salt;
            int64_t loginId;
            int passlen;
            int saltlen;
            int rc;

            rc = db_read(db, stmt);
            
            switch (rc)
            {
            case SQLITE_ROW:
                loginId = db_fetch_int64(stmt, 0);
                passhash = db_fetch_blob(stmt, 1, &passlen);
                salt = db_fetch_blob(stmt, 2, &saltlen);
            
                if (loginId < 0 || !passhash || !salt || passlen != LOGIN_CRYPTO_HASH_SIZE || saltlen != LOGIN_CRYPTO_SALT_SIZE)
                    break;
                
                reply.db.zResult.hadError = false;
                reply.db.zResult.rLoginCredentials.loginId = loginId;
                memcpy(reply.db.zResult.rLoginCredentials.passwordHash, passhash, LOGIN_CRYPTO_HASH_SIZE);
                memcpy(reply.db.zResult.rLoginCredentials.salt, salt, LOGIN_CRYPTO_SALT_SIZE);
                break;
            
            case SQLITE_DONE:
                reply.db.zResult.hadError = false;
                break;
            
            case SQLITE_ERROR:
                break;
            }
        }
    }
    
    db_reply(db, zpacket, &reply);
    sqlite3_finalize(stmt);
}

void dbr_dispatch(DbThread* db, sqlite3* sqlite, ZPacket* zpacket)
{
    uint64_t timer = clock_microseconds();
    int zop = zpacket->db.zQuery.zop;
    
    switch (zop)
    {
    case ZOP_DB_QueryLoginCredentials:
        dbr_login_credentials(db, sqlite, zpacket);
        break;
    
    default:
        log_writef(db_get_log_queue(db), db_get_log_id(db), "WARNING: dbr_dispatch: received unexpected zop: %s", enum2str_zop(zop));
        goto destruct;
    }
    
    timer = clock_microseconds() - timer;
    log_writef(db_get_log_queue(db), db_get_log_id(db), "Executed READ query %s in %llu microseconds", enum2str_zop(zop), timer);
destruct:
    dbr_destruct(db, zpacket, zop);
}

void dbr_destruct(DbThread* db, ZPacket* zpacket, int zop)
{
    switch (zop)
    {
    case ZOP_DB_QueryLoginCredentials:
        zpacket->db.zQuery.qLoginCredentials.accountName = sbuf_drop(zpacket->db.zQuery.qLoginCredentials.accountName);
        break;
    
    default:
        log_writef(db_get_log_queue(db), db_get_log_id(db), "WARNING: dbr_destruct: received unexpected zop: %s", enum2str_zop(zop));
        break;
    }
}
