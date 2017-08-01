
#include "db_read.h"
#include "bit.h"
#include "login_crypto.h"
#include "util_alloc.h"
#include "util_clock.h"
#include "enum_zop.h"
#include "enum2str.h"

static void dbr_main_guild_list(DbThread* db, sqlite3* sqlite, ZPacket* zpacket)
{
    sqlite3_stmt* stmt = NULL;
    ZPacket reply;
    bool run;

    reply.db.zResult.queryId = zpacket->db.zQuery.queryId;
    reply.db.zResult.hadError = true;
    reply.db.zResult.hadErrorUnprocessed = false;
    reply.db.zResult.rMainGuildList.count = 0;
    reply.db.zResult.rMainGuildList.guilds = NULL;

    run = db_prepare_literal(db, sqlite, &stmt,
        "SELECT guild_id, name FROM guild LIMIT 512");

    if (run)
    {
        uint32_t index = 0;
        Guild* guilds = NULL;

        for (;;)
        {
            int rc = db_read(db, stmt);

            switch (rc)
            {
            case SQLITE_ROW:
                if (bit_is_pow2_or_zero(index))
                {
                    uint32_t cap = (index == 0) ? 1 : index * 2;
                    Guild* newGuilds = realloc_array_type(guilds, cap, Guild);

                    if (!newGuilds) goto error;

                    guilds = newGuilds;
                }

                guilds[index].guildId = db_fetch_int(stmt, 0);
                guilds[index].guildName = db_fetch_string_copy(stmt, 1);
                index++;

                if (!guilds[index].guildName) goto error;

                break;

            case SQLITE_DONE:
                reply.db.zResult.hadError = false;
                reply.db.zResult.rMainGuildList.count = index;
                reply.db.zResult.rMainGuildList.guilds = guilds;
                goto done;

            case SQLITE_ERROR:
            error:
                if (guilds)
                {
                    uint32_t i;

                    for (i = 0; i < index; i++)
                    {
                        guilds[i].guildName = sbuf_drop(guilds[i].guildName);
                    }

                    free(guilds);
                }

                goto done;
            }
        }
    }

done:
    db_reply(db, zpacket, &reply);
    sqlite3_finalize(stmt);
}

static void dbr_login_credentials(DbThread* db, sqlite3* sqlite, ZPacket* zpacket)
{
    sqlite3_stmt* stmt = NULL;
    ZPacket reply;
    bool run;
    
    reply.db.zResult.queryId = zpacket->db.zQuery.queryId;
    reply.db.zResult.hadError = true;
    reply.db.zResult.hadErrorUnprocessed = false;
    reply.db.zResult.rLoginCredentials.client = zpacket->db.zQuery.qLoginCredentials.client;
    reply.db.zResult.rLoginCredentials.acctId = -1;
    reply.db.zResult.rLoginCredentials.status = 0;
    reply.db.zResult.rLoginCredentials.suspendedUntil = 0;
    
    run = db_prepare_literal(db, sqlite, &stmt,
        "SELECT id, password_hash, salt, status, suspended_until FROM account WHERE name = ? LIMIT 1");
    
    if (run)
    {
        StaticBuffer* acctName = zpacket->db.zQuery.qLoginCredentials.accountName;
        
        if (db_bind_string_sbuf(db, stmt, 0, acctName))
        {
            const byte* passhash;
            const byte* salt;
            int64_t acctId;
            int passlen;
            int saltlen;
            int rc;

            rc = db_read(db, stmt);
            
            switch (rc)
            {
            case SQLITE_ROW:
                acctId = db_fetch_int64(stmt, 0);
                passhash = db_fetch_blob(stmt, 1, &passlen);
                salt = db_fetch_blob(stmt, 2, &saltlen);
            
                if (acctId < 0 || !passhash || !salt || passlen != LOGIN_CRYPTO_HASH_SIZE || saltlen != LOGIN_CRYPTO_SALT_SIZE)
                    break;
                
                reply.db.zResult.hadError = false;
                reply.db.zResult.rLoginCredentials.acctId = acctId;
                memcpy(reply.db.zResult.rLoginCredentials.passwordHash, passhash, LOGIN_CRYPTO_HASH_SIZE);
                memcpy(reply.db.zResult.rLoginCredentials.salt, salt, LOGIN_CRYPTO_SALT_SIZE);
                reply.db.zResult.rLoginCredentials.status = db_fetch_int(stmt, 3);
                reply.db.zResult.rLoginCredentials.suspendedUntil = db_fetch_int64(stmt, 4);
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

static void dbr_cs_character_info(DbThread* db, sqlite3* sqlite, ZPacket* zpacket)
{
    sqlite3_stmt* stmt = NULL;
    ZPacket reply;
    bool run;
    
    reply.db.zResult.queryId = zpacket->db.zQuery.queryId;
    reply.db.zResult.hadError = true;
    reply.db.zResult.hadErrorUnprocessed = false;
    reply.db.zResult.rCSCharacterInfo.count = 0;
    reply.db.zResult.rCSCharacterInfo.client = zpacket->db.zQuery.qCSCharacterInfo.client;
    reply.db.zResult.rCSCharacterInfo.data = NULL;
    
    run = db_prepare_literal(db, sqlite, &stmt,
        "SELECT "
            "name, level, class, race, zone_id, gender, face, "
            "material0, material1, material2, material3, material4, material5, material6, material7, material8, "
            "tint0, tint1, tint2, tint3, tint4, tint5, tint6 "
        "FROM character "
        "WHERE account_id = ? "
        "ORDER BY character_id ASC LIMIT 10");
    
    if (run && db_bind_int64(db, stmt, 0, zpacket->db.zQuery.qCSCharacterInfo.accountId))
    {
        CharSelectData* data = alloc_type(CharSelectData);
        uint32_t index = 0;
        uint32_t i;
        
        if (!data) goto done;
        
        memset(data, 0, sizeof(CharSelectData));
        
        for (;;)
        {
            int rc = db_read(db, stmt);
            
            switch (rc)
            {
            case SQLITE_ROW:
                data->name[index] = db_fetch_string_copy(stmt, 0);
                data->level[index] = db_fetch_int(stmt, 1);
                data->classId[index] = db_fetch_int(stmt, 2);
                data->raceId[index] = db_fetch_int(stmt, 3);
                data->zoneId[index] = db_fetch_int(stmt, 4);
                data->genderId[index] = db_fetch_int(stmt, 5);
                data->faceId[index] = db_fetch_int(stmt, 6);
            
                data->materialIds[index][0] = db_fetch_int(stmt, 7);
                data->materialIds[index][1] = db_fetch_int(stmt, 8);
                data->materialIds[index][2] = db_fetch_int(stmt, 9);
                data->materialIds[index][3] = db_fetch_int(stmt, 10);
                data->materialIds[index][4] = db_fetch_int(stmt, 11);
                data->materialIds[index][5] = db_fetch_int(stmt, 12);
                data->materialIds[index][6] = db_fetch_int(stmt, 13);
                data->materialIds[index][7] = db_fetch_int(stmt, 14);
                data->materialIds[index][8] = db_fetch_int(stmt, 15);
            
                data->materialTints[index][0] = (uint32_t)db_fetch_int64(stmt, 16);
                data->materialTints[index][1] = (uint32_t)db_fetch_int64(stmt, 17);
                data->materialTints[index][2] = (uint32_t)db_fetch_int64(stmt, 18);
                data->materialTints[index][3] = (uint32_t)db_fetch_int64(stmt, 19);
                data->materialTints[index][4] = (uint32_t)db_fetch_int64(stmt, 20);
                data->materialTints[index][5] = (uint32_t)db_fetch_int64(stmt, 21);
                data->materialTints[index][6] = (uint32_t)db_fetch_int64(stmt, 22);
            
                if (!data->name[index]) goto error;
                
                index++;
                break;
            
            case SQLITE_DONE:
                reply.db.zResult.hadError = false;
                reply.db.zResult.rCSCharacterInfo.count = index;
                reply.db.zResult.rCSCharacterInfo.data = data;
                goto done;
            
            case SQLITE_ERROR:
            error:
                for (i = 0; i < 10; i++)
                {
                    data->name[i] = sbuf_drop(data->name[i]);
                }
                
                free(data);
                goto done;
            }
        }
    }
    
done:
    db_reply(db, zpacket, &reply);
    sqlite3_finalize(stmt);
}

void dbr_dispatch(DbThread* db, sqlite3* sqlite, ZPacket* zpacket)
{
    uint64_t timer = clock_microseconds();
    int zop = zpacket->db.zQuery.zop;
    
    switch (zop)
    {
    case ZOP_DB_QueryMainGuildList:
        break;

    case ZOP_DB_QueryLoginCredentials:
        dbr_login_credentials(db, sqlite, zpacket);
        break;
    
    case ZOP_DB_QueryCSCharacterInfo:
        dbr_cs_character_info(db, sqlite, zpacket);
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
    
    /* No data to destruct */
    case ZOP_DB_QueryMainGuildList:
    case ZOP_DB_QueryCSCharacterInfo:
        break;
    
    default:
        log_writef(db_get_log_queue(db), db_get_log_id(db), "WARNING: dbr_destruct: received unexpected zop: %s", enum2str_zop(zop));
        break;
    }
}
