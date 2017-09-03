
#include "db_write.h"
#include "client_save.h"
#include "item_proto.h"
#include "login_crypto.h"
#include "skills.h"
#include "spellbook.h"
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
    uint32_t n, i;

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
            "surname = ?, name = ?, level = ?, class = ?, race = ?, zone_id = ?, inst_id = ?, gender = ?, face = ?, deity = ?, "
            "x = ?, y = ?, z = ?, heading = ?, "
            "current_hp = ?, current_mana = ?, experience = ?, training_points = ?, guild_id = ?, guild_rank = ?, "
            "harmtouch_timestamp = ?, discipline_timestamp = ?, pp = ?, gp = ?, sp = ?, cp = ?, "
            "pp_cursor = ?, gp_cursor = ?, sp_cursor = ?, cp_cursor = ?, pp_bank = ?, gp_bank = ?, sp_bank = ?, cp_bank = ?, "
            "hunger = ?, thirst = ?, is_gm = ?, autosplit = ?, is_pvp = ?, anon = ?, drunkeness = ?, "
            "material0 = ?, material1 = ?, material2 = ?, material3 = ?, material4 = ?, material5 = ?, material6 = ?, "
            "material7 = ?, material8 = ?, tint0 = ?, tint1 = ?, tint2 = ?, tint3 = ?, tint4 = ?, tint5 = ?, tint6 = ? "
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
            db_bind_double(db, stmt, 12, save->z) &&
            db_bind_double(db, stmt, 13, save->heading) &&
            db_bind_int64(db, stmt, 14, save->curHp) &&
            db_bind_int64(db, stmt, 15, save->curMana) &&
            db_bind_int64(db, stmt, 16, save->experience) &&
            db_bind_int(db, stmt, 17, save->trainingPoints) &&
            db_bind_int(db, stmt, 18, save->guildId) &&
            db_bind_int(db, stmt, 19, save->guildRank) &&
            db_bind_int64(db, stmt, 20, (int64_t)save->harmtouchTimestamp) &&
            db_bind_int64(db, stmt, 21, (int64_t)save->disciplineTimestamp) &&
            db_bind_int(db, stmt, 22, save->coin.pp) &&
            db_bind_int(db, stmt, 23, save->coin.gp) &&
            db_bind_int(db, stmt, 24, save->coin.sp) &&
            db_bind_int(db, stmt, 25, save->coin.cp) &&
            db_bind_int(db, stmt, 26, save->coinCursor.pp) &&
            db_bind_int(db, stmt, 27, save->coinCursor.gp) &&
            db_bind_int(db, stmt, 28, save->coinCursor.sp) &&
            db_bind_int(db, stmt, 29, save->coinCursor.cp) &&
            db_bind_int(db, stmt, 30, save->coinBank.pp) &&
            db_bind_int(db, stmt, 31, save->coinBank.gp) &&
            db_bind_int(db, stmt, 32, save->coinBank.sp) &&
            db_bind_int(db, stmt, 33, save->coinBank.cp) &&
            db_bind_int(db, stmt, 34, save->hunger) &&
            db_bind_int(db, stmt, 35, save->thirst) &&
            db_bind_int(db, stmt, 36, save->isGM) &&
            db_bind_int(db, stmt, 37, save->isAutosplit) &&
            db_bind_int(db, stmt, 38, save->isPvP) &&
            db_bind_int(db, stmt, 39, save->anonSetting) &&
            db_bind_int(db, stmt, 40, save->drunkeness) &&
            db_bind_int(db, stmt, 41, save->materialIds[0]) &&
            db_bind_int(db, stmt, 42, save->materialIds[1]) &&
            db_bind_int(db, stmt, 43, save->materialIds[2]) &&
            db_bind_int(db, stmt, 44, save->materialIds[3]) &&
            db_bind_int(db, stmt, 45, save->materialIds[4]) &&
            db_bind_int(db, stmt, 46, save->materialIds[5]) &&
            db_bind_int(db, stmt, 47, save->materialIds[6]) &&
            db_bind_int(db, stmt, 48, save->materialIds[7]) &&
            db_bind_int(db, stmt, 49, save->materialIds[8]) &&
            db_bind_int(db, stmt, 50, save->tints[0]) &&
            db_bind_int(db, stmt, 51, save->tints[1]) &&
            db_bind_int(db, stmt, 52, save->tints[2]) &&
            db_bind_int(db, stmt, 53, save->tints[3]) &&
            db_bind_int(db, stmt, 54, save->tints[4]) &&
            db_bind_int(db, stmt, 55, save->tints[5]) &&
            db_bind_int(db, stmt, 56, save->tints[6]) &&
            db_bind_int64(db, stmt, 57, save->characterId))
        {
            db_write(db, stmt);
        }
    }
    
    sqlite3_finalize(stmt);
    stmt = NULL;

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
    run = db_prepare_literal(db, sqlite, &stmt, "INSERT INTO skill (character_id, skill_id, value) VALUES (?, ?, ?)");
    
    if (run && db_bind_int64(db, stmt, 0, charId))
    {
        Skills* sk = &save->skills;
        
        for (i = 0; i < SKILL_COUNT; i++)
        {
            uint8_t val = sk->skill[i];
            
            if (!skill_is_learned_value(val))
                continue;
            
            if (db_bind_int(db, stmt, 1, (int)i) && db_bind_int(db, stmt, 2, val))
            {
                db_write(db, stmt);
            }
            
            sqlite3_reset(stmt);
        }
        
        for (i = 0; i < LANG_COUNT; i++)
        {
            uint8_t val = sk->language[i];
            
            if (!skill_is_learned_value(val))
                continue;
            
            if (db_bind_int(db, stmt, 1, (int)(i + SKILL_LANGUAGE_DB_OFFSET)) && db_bind_int(db, stmt, 2, val))
            {
                db_write(db, stmt);
            }
            
            sqlite3_reset(stmt);
        }
    }
    
    sqlite3_finalize(stmt);
    stmt = NULL;

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
    run = db_prepare_literal(db, sqlite, &stmt, 
        "INSERT INTO bind_point "
            "(character_id, bind_id, zone_id, x, y, z, heading) "
        "VALUES "
            "(?, ?, ?, ?, ?, ?, ?)");
    
    if (run && db_bind_int64(db, stmt, 0, charId))
    {
        BindPoint* binds = save->bindPoints;
        
        for (i = 0; i < 5; i++)
        {
            BindPoint* bind = &binds[i];
            
            if (bind->zoneId == 0)
                continue;
            
            if (db_bind_int(db, stmt, 1, i) &&
                db_bind_int(db, stmt, 2, bind->zoneId) &&
                db_bind_double(db, stmt, 3, bind->loc.x) &&
                db_bind_double(db, stmt, 4, bind->loc.y) &&
                db_bind_double(db, stmt, 5, bind->loc.z) &&
                db_bind_double(db, stmt, 6, bind->loc.heading))
            {
                db_write(db, stmt);
            }
            
            sqlite3_reset(stmt);
        }
    }
    
    sqlite3_finalize(stmt);
    stmt = NULL;

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
    run = db_prepare_literal(db, sqlite, &stmt,
        "INSERT INTO memmed_spells "
            "(character_id, slot_id, spell_id, recast_timestamp_ms) "
        "VALUES "
            "(?, ?, ?, ?)");
    
    if (run && db_bind_int64(db, stmt, 0, charId))
    {
        ClientSaveMemmedSpell* memmed = save->memmedSpells;
        
        for (i = 0; i < MEMMED_SPELL_SLOTS; i++)
        {
            ClientSaveMemmedSpell* mem = &memmed[i];
            
            if (db_bind_int(db, stmt, 1, i) && db_bind_int(db, stmt, 2, mem->spellId) && db_bind_int64(db, stmt, 3, (int64_t)mem->recastTimestamp))
            {
                db_write(db, stmt);
            }
            
            sqlite3_reset(stmt);
        }
    }
    
    sqlite3_finalize(stmt);
    stmt = NULL;

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
    if (save->spellbook)
    {
        run = db_prepare_literal(db, sqlite, &stmt,
            "INSERT INTO spellbook "
                "(character_id, slot_id, spell_id) "
            "VALUES "
                "(?, ?, ?)");
        
        if (run && db_bind_int64(db, stmt, 0, charId))
        {
            SpellbookSlot* spellbook = save->spellbook;
            
            n = save->spellbookCount;
            
            for (i = 0; i < n; i++)
            {
                SpellbookSlot* spell = &spellbook[i];
                
                if (db_bind_int(db, stmt, 1, spell->slotId) && db_bind_int(db, stmt, 2, spell->spellId))
                {
                    db_write(db, stmt);
                }
                
                sqlite3_reset(stmt);
            }
        }
    }

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
    if (save->items)
    {
        run = db_prepare_literal(db, sqlite, &stmt,
            "INSERT INTO inventory "
                "(character_id, slot_id, item_id, stack_amount, charges) "
            "VALUES "
                "(?, ?, ?, ?, ?)");
        
        if (run && db_bind_int64(db, stmt, 0, charId))
        {
            ClientSaveInvSlot* items = save->items;
            
            n = save->itemCount;
            
            for (i = 0; i < n; i++)
            {
                ClientSaveInvSlot* item = &items[i];
                
                if (db_bind_int(db, stmt, 1, item->slotId) &&
                    db_bind_int64(db, stmt, 2, item->itemId) &&
                    db_bind_int(db, stmt, 3, item->stackAmt) &&
                    db_bind_int(db, stmt, 4, item->charges))
                {
                    db_write(db, stmt);
                }
                
                sqlite3_reset(stmt);
            }
        }
    }

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
    if (save->cursorQueue)
    {
        run = db_prepare_literal(db, sqlite, &stmt,
            "INSERT INTO cursor_queue "
                "(character_id, item_id, stack_amount, charges) "
            "VALUES "
                "(?, ?, ?, ?)");
        
        if (run && db_bind_int64(db, stmt, 0, charId))
        {
            ClientSaveInvSlot* items = save->cursorQueue;
            
            n = save->cursorQueueCount;
            
            for (i = 0; i < n; i++)
            {
                ClientSaveInvSlot* item = &items[i];
                
                if (db_bind_int64(db, stmt, 1, item->itemId) &&
                    db_bind_int(db, stmt, 2, item->stackAmt) &&
                    db_bind_int(db, stmt, 3, item->charges))
                {
                    db_write(db, stmt);
                }
                
                sqlite3_reset(stmt);
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
