
#include "db_write.h"
#include "login_crypto.h"
#include "util_clock.h"
#include "util_str.h"
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
    
    run = db_prepare_literal(db, sqlite, &stmt,
        "INSERT INTO account "
            "(name, password_hash, salt, creation_time) "
        "VALUES "
            "(?, ?, ?, datetime('now'))");
    
    if (run)
    {
        StaticBuffer* acctName = zpacket->db.zQuery.qLoginNewAccount.accountName;
        
        if (db_bind_string_sbuf(db, stmt, 0, acctName) &&
            db_bind_blob(db, stmt, 1, zpacket->db.zQuery.qLoginNewAccount.passwordHash, LOGIN_CRYPTO_HASH_SIZE) &&
            db_bind_blob(db, stmt, 2, zpacket->db.zQuery.qLoginNewAccount.salt, LOGIN_CRYPTO_SALT_SIZE))
        {
            if (db_write(db, stmt))
            {
                reply.db.zResult.hadError = false;
                reply.db.zResult.rLoginNewAccount.acctId = sqlite3_last_insert_rowid(sqlite);
            }
        }
    }
    
    db_reply(db, zpacket, &reply);
    sqlite3_finalize(stmt);
}

static void dbw_cs_character_create(DbThread* db, sqlite3* sqlite, ZPacket* zpacket)
{
    sqlite3_stmt* stmt = NULL;
    ZPacket reply;
    bool run;

    reply.db.zResult.queryId = zpacket->db.zQuery.queryId;
    reply.db.zResult.hadError = true;
    reply.db.zResult.hadErrorUnprocessed = false;
    reply.db.zResult.rCSCharacterCreate.client = zpacket->db.zQuery.qCSCharacterCreate.client;

    run = db_prepare_literal(db, sqlite, &stmt,
        "INSERT INTO character "
            "(account_id, name, gender, race, class, face, current_hp, base_str, base_sta, base_cha, base_dex,"
            " base_int, base_agi, base_wis, deity, zone_id, x, y, z, heading, creation_time) "
        "VALUES "
            "(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, datetime('now'))");
    
    if (run)
    {
        CharCreateData* data = zpacket->db.zQuery.qCSCharacterCreate.data;

        if (
            db_bind_int64(db, stmt, 0, zpacket->db.zQuery.qCSCharacterCreate.accountId) &&
            db_bind_string(db, stmt, 1, data->name, str_len_bounded(data->name, sizeof_field(CharCreateData, name))) &&
            db_bind_int(db, stmt, 2, data->genderId) &&
            db_bind_int(db, stmt, 3, data->raceId) &&
            db_bind_int(db, stmt, 4, data->classId) &&
            db_bind_int(db, stmt, 5, data->faceId) &&
            db_bind_int(db, stmt, 6, data->currentHp) &&
            db_bind_int(db, stmt, 7, data->STR) &&
            db_bind_int(db, stmt, 8, data->STA) &&
            db_bind_int(db, stmt, 9, data->CHA) &&
            db_bind_int(db, stmt, 10, data->DEX) &&
            db_bind_int(db, stmt, 11, data->INT) &&
            db_bind_int(db, stmt, 12, data->AGI) &&
            db_bind_int(db, stmt, 13, data->WIS) &&
            db_bind_int(db, stmt, 14, data->deityId) &&
            db_bind_int(db, stmt, 15, data->zoneId) &&
            db_bind_double(db, stmt, 16, data->x) &&
            db_bind_double(db, stmt, 17, data->y) &&
            db_bind_double(db, stmt, 18, data->z) &&
            db_bind_double(db, stmt, 19, data->heading)
           )
        {
            if (db_write(db, stmt))
            {
                reply.db.zResult.hadError = false;
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

    case ZOP_DB_QueryCSCharacterCreate:
        dbw_cs_character_create(db, sqlite, zpacket);
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

    case ZOP_DB_QueryCSCharacterCreate:
        free(zpacket->db.zQuery.qCSCharacterCreate.data);
        break;
    
    default:
        log_writef(db_get_log_queue(db), db_get_log_id(db), "WARNING: dbw_destruct: received unexpected zop: %s", enum2str_zop(zop));
        break;
    }
}
