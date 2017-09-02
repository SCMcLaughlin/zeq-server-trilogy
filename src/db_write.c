
#include "db_write.h"
#include "client_save.h"
#include "item_proto.h"
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

static void dbw_cs_character_delete(DbThread* db, sqlite3* sqlite, ZPacket* zpacket)
{
    sqlite3_stmt* stmt = NULL;
    ZPacket reply;
    bool run;

    reply.db.zResult.queryId = zpacket->db.zQuery.queryId;
    reply.db.zResult.hadError = true;
    reply.db.zResult.hadErrorUnprocessed = false;
    reply.db.zResult.rCSCharacterDelete.client = zpacket->db.zQuery.qCSCharacterDelete.client;

    run = db_prepare_literal(db, sqlite, &stmt,
        "DELETE FROM character WHERE name = ? AND account_id = ?");

    if (run && db_bind_string_sbuf(db, stmt, 0, zpacket->db.zQuery.qCSCharacterDelete.name) && db_bind_int64(db, stmt, 1, zpacket->db.zQuery.qCSCharacterDelete.accountId))
    {
        if (db_write(db, stmt))
        {
            reply.db.zResult.hadError = false;
        }
    }

    db_reply(db, zpacket, &reply);
    sqlite3_finalize(stmt);
}

static void dbw_main_item_proto_changes(DbThread* db, sqlite3* sqlite, ZPacket* zpacket)
{
    sqlite3_stmt* stmt = NULL;
    ZPacket reply;
    uint32_t count = zpacket->db.zQuery.qMainItemProtoChanges.count;
    ItemProtoDb* protos = zpacket->db.zQuery.qMainItemProtoChanges.proto;
    uint32_t i;
    bool run;

    reply.db.zResult.queryId = zpacket->db.zQuery.queryId;
    reply.db.zResult.hadError = true;
    reply.db.zResult.hadErrorUnprocessed = false;
    
    if (count && protos)
    {
        run = db_prepare_literal(db, sqlite, &stmt,
        "INSERT OR REPLACE INTO item_proto (item_id, path, mod_time, name, lore_text, data) VALUES (?, ?, ?, ?, ?, ?)");
        
        if (!run) goto fail;
        
        for (i = 0; i < count; i++)
        {
            ItemProtoDb* cur = &protos[i];
            ItemProto* proto = cur->proto;
            StaticBuffer* name = item_proto_name(proto);
            StaticBuffer* lore = item_proto_lore_text(proto);
            
            if (!db_bind_int64(db, stmt, 0, (int64_t)cur->itemId) ||
                !db_bind_string_sbuf(db, stmt, 1, cur->path) ||
                !db_bind_int64(db, stmt, 2, (int64_t)cur->modTime) ||
                !(name ? db_bind_string_sbuf(db, stmt, 3, name) : db_bind_null(db, stmt, 3)) ||
                !(lore ? db_bind_string_sbuf(db, stmt, 4, lore) : db_bind_null(db, stmt, 4)) ||
                !db_bind_blob(db, stmt, 5, proto, item_proto_bytes(proto)))
            {
                goto fail;
            }
            
            if (!db_write(db, stmt))
                goto fail;
            
            sqlite3_reset(stmt);
        }
    }
    
    reply.db.zResult.hadError = false;
    
fail:
    db_reply(db, zpacket, &reply);
    sqlite3_finalize(stmt);
}

static void dbw_main_item_proto_deletes(DbThread* db, sqlite3* sqlite, ZPacket* zpacket)
{
    sqlite3_stmt* stmt = NULL;
    ZPacket reply;
    uint32_t count = zpacket->db.zQuery.qMainItemProtoDeletes.count;
    uint32_t* itemIds = zpacket->db.zQuery.qMainItemProtoDeletes.itemIds;
    uint32_t i;

    reply.db.zResult.queryId = zpacket->db.zQuery.queryId;
    reply.db.zResult.hadError = true;
    reply.db.zResult.hadErrorUnprocessed = false;

    if (count && itemIds)
    {
        bool run = db_prepare_literal(db, sqlite, &stmt,
            "DELETE FROM item_proto WHERE item_id = ?");

        if (!run) goto fail;

        for (i = 0; i < count; i++)
        {
            if (!db_bind_int64(db, stmt, 0, (int64_t)itemIds[i]))
                goto fail;

            if (!db_write(db, stmt))
                goto fail;

            sqlite3_reset(stmt);
        }
    }

    reply.db.zResult.hadError = false;

fail:
    db_reply(db, zpacket, &reply);
    sqlite3_finalize(stmt);
}

static void dbw_zone_client_save(DbThread* db, sqlite3* sqlite, ZPacket* zpacket)
{
    sqlite3_stmt* stmt = NULL;
    ZPacket reply;
    bool run;
    ClientSave* save = zpacket->db.zQuery.qZoneClientSave.save;
    int64_t charId = save->characterId;

    reply.db.zResult.queryId = zpacket->db.zQuery.queryId;
    reply.db.zResult.hadError = true;
    reply.db.zResult.hadErrorUnprocessed = false;

    /*
        Mutli-query transaction:
         1) UPDATE character
         2) DELETE skills
         3) INSERT skills
         4) DELETE bind_point
         5) INSERT bind_point
         6) DELETE memmed_spells
         7) INSERT memmed_spells
         8) DELETE spellbook
         9) INSERT spellbook
        10) DELETE inventory
        11) INSERT inventory
        12) DELETE cursor_queue
        13) INSERT cursor_queue
    */

    /* UPDATE character */
    run = db_prepare_literal(db, sqlite, &stmt,
        "UPDATE character SET "
            "surname = ?, name = ?, level = ?, class = ?, race = ?, zone_id = ?, inst_id = ?, gender = ?, face = ?, deity = ?, x = ?, y = ?, z = ?, heading = ?, "
            "current_hp = ?, current_mana = ?, experience = ?, training_points = ?, guild_id = ?, guild_rank = ?, "
            "harmtouch_timestamp = ?, discipline_timestamp = ?, pp = ?, gp = ?, sp = ?, cp = ?, "
            "pp_cursor = ?, gp_cursor = ?, sp_cursor = ?, cp_cursor = ?, pp_bank = ?, gp_bank = ?, sp_bank = ?, cp_bank = ?, "
            "hunger = ?, thirst = ?, is_gm = ?, autosplit = ?, is_pvp = ?, anon = ?, drunkeness = ?, "
            "material0 = ?, material1 = ?, material2 = ?, material3 = ?, material4 = ? material5 = ?, material6 = ?, "
            "material7 = ?, material8 = ?, tint0 = ?, tint1 = ?, tint2 = ?, tint3 = ?, tint4 = ? tint5 = ?, tint 6 = ? "
        "WHERE character_id = ?");

    if (run)
    {
        bool surnameGood = (save->surname) ? db_bind_string_sbuf(db, stmt, 0, save->surname) : db_bind_null(db, stmt, 0);

        if (surnameGood &&
            db_bind_string_sbuf(db, stmt, 1, save->name) &&
            db_bind_int(db, stmt, 2, save->level) &&
            db_bind_int(db, stmt, 3, save->classId) &&
            db_bind_int(db, stmt, 4, save->raceId) &&
            db_bind_int(db, stmt, 5, save->zoneId) &&
            db_bind_int(db, stmt, 6, save->instId) &&
            db_bind_int(db, stmt, 7, save->genderId) &&
            db_bind_int(db, stmt, 8, save->faceId) &&
            db_bind_int(db, stmt, 9, save->deityId) &&
            db_bind_double(db, stmt, 10, save->x) &&
            db_bind_double(db, stmt, 11, save->y) &&
            db_bind_double(db, stmt, 12, save->z)
        )
        {

        }
    }

    /* DELETE skills */
    run = db_prepare_literal(db, sqlite, &stmt, "DELETE FROM skill WHERE character_id = ?");
    if (run)
    {
        if (db_bind_int64(db, stmt, 0, charId))
        {
            db_write(db, stmt);   
        }
    }
    sqlite3_finalize(stmt);
    stmt = NULL;

    /* INSERT skills */

    /* DELETE bind_point */
    run = db_prepare_literal(db, sqlite, &stmt, "DELETE FROM bind_point WHERE character_id = ?");
    if (run)
    {
        if (db_bind_int64(db, stmt, 0, charId))
        {
            db_write(db, stmt);   
        }
    }
    sqlite3_finalize(stmt);
    stmt = NULL;

    /* INSERT bind_point */

    /* DELETE memmed_spells */
    run = db_prepare_literal(db, sqlite, &stmt, "DELETE FROM memmed_spells WHERE character_id = ?");
    if (run)
    {
        if (db_bind_int64(db, stmt, 0, charId))
        {
            db_write(db, stmt);   
        }
    }
    sqlite3_finalize(stmt);
    stmt = NULL;

    /* INSERT memmed_spells */

    /* DELETE spellbook */
    run = db_prepare_literal(db, sqlite, &stmt, "DELETE FROM spellbook WHERE character_id = ?");
    if (run)
    {
        if (db_bind_int64(db, stmt, 0, charId))
        {
            db_write(db, stmt);   
        }
    }
    sqlite3_finalize(stmt);
    stmt = NULL;

    /* INSERT spellbook */

    /* DELETE inventory */
    run = db_prepare_literal(db, sqlite, &stmt, "DELETE FROM inventory WHERE character_id = ?");
    if (run)
    {
        if (db_bind_int64(db, stmt, 0, charId))
        {
            db_write(db, stmt);   
        }
    }
    sqlite3_finalize(stmt);
    stmt = NULL;

    /* INSERT inventory */

    /* DELETE cursor_queue */
    run = db_prepare_literal(db, sqlite, &stmt, "DELETE FROM cursor_queue WHERE character_id = ?");
    if (run)
    {
        if (db_bind_int64(db, stmt, 0, charId))
        {
            db_write(db, stmt);   
        }
    }
    sqlite3_finalize(stmt);
    stmt = NULL;

    /* INSERT cursor_queue */

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

    case ZOP_DB_QueryCSCharacterDelete:
        dbw_cs_character_delete(db, sqlite, zpacket);
        break;
    
    case ZOP_DB_QueryMainItemProtoChanges:
        dbw_main_item_proto_changes(db, sqlite, zpacket);
        break;

    case ZOP_DB_QueryMainItemProtoDeletes:
        dbw_main_item_proto_deletes(db, sqlite, zpacket);
        break;

    case ZOP_DB_QueryZoneClientSave:
        dbw_zone_client_save(db, sqlite, zpacket);
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
        free_if_exists(zpacket->db.zQuery.qCSCharacterCreate.data);
        break;

    case ZOP_DB_QueryCSCharacterDelete:
        zpacket->db.zQuery.qCSCharacterDelete.name = sbuf_drop(zpacket->db.zQuery.qCSCharacterDelete.name);
        break;
    
    case ZOP_DB_QueryMainItemProtoChanges:
        free_if_exists(zpacket->db.zQuery.qMainItemProtoChanges.proto);
        break;

    case ZOP_DB_QueryMainItemProtoDeletes:
        free_if_exists(zpacket->db.zQuery.qMainItemProtoDeletes.itemIds);
        break;

    case ZOP_DB_QueryZoneClientSave:
        client_save_destroy(zpacket->db.zQuery.qZoneClientSave.save);
        break;
    
    default:
        log_writef(db_get_log_queue(db), db_get_log_id(db), "WARNING: dbw_destruct: received unexpected zop: %s", enum2str_zop(zop));
        break;
    }
}
