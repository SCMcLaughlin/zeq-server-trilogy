
#include "main_load.h"
#include "bit.h"
#include "db_thread.h"
#include "main_thread.h"
#include "ringbuf.h"
#include "util_alloc.h"
#include "util_clock.h"
#include "util_fs.h"
#include "zpacket.h"
#include "enum_zop.h"

enum MainLoadProgress
{
    MAIN_LOAD_ItemProtos,
    MAIN_LOAD_ItemProtoChanges,
    MAIN_LOAD_ItemProtoDeletes,
    MAIN_LOAD_COUNT
};

static int mt_load_check_db_error(MainThread* mt, ZPacket* zpacket, const char* funcName)
{
    if (zpacket->db.zResult.hadErrorUnprocessed)
    {
        log_writef(mt_get_log_queue(mt), mt_get_log_id(mt), "WARNING: %s: database reported error, query unprocessed", funcName);
        return ERR_Invalid;
    }

    if (zpacket->db.zResult.hadError)
    {
        log_writef(mt_get_log_queue(mt), mt_get_log_id(mt), "WARNING: %s: database reported error", funcName);
        return ERR_Generic;
    }

    return ERR_None;
}

static int mt_load_item_protos(MainThread* mt, ZPacket* zpacket, uint32_t* progress)
{
    uint32_t count, i;
    uint64_t time;
    ItemProtoDb* protos;
    ItemList* itemList;
    lua_State* L;
    uint32_t useCount = 0;
    uint32_t replaceCount = 0;
    RingBuf* logQueue = mt_get_log_queue(mt);
    int logId = mt_get_log_id(mt);
    ItemProtoDbChanges changes;
    uint32_t delCount = 0;
    uint32_t* delItemIds = NULL;
    char pathbuf[1024];
    ZPacket query;
    int rc;

    rc = mt_load_check_db_error(mt, zpacket, FUNC_NAME);
    if (rc) goto fail_db;
    
    *progress |= nth_bit(MAIN_LOAD_ItemProtos);

    changes.count = 0;
    changes.proto = NULL;

    count = zpacket->db.zResult.rMainLoadItemProtos.count;
    protos = zpacket->db.zResult.rMainLoadItemProtos.protos;

    L = mt_get_lua(mt);
    itemList = mt_get_item_list(mt);

    itemList->nextItemId = zpacket->db.zResult.rMainLoadItemProtos.highItemId + 1;

    lua_newtable(L);
    
#define BASE_PATH "script/item/"
    memcpy(pathbuf, BASE_PATH, sizeof(BASE_PATH));

    for (i = 0; i < count; i++)
    {
        ItemProtoDb* cur = &protos[i];
        uint64_t mtime;
        
        snprintf(pathbuf + sizeof(BASE_PATH) - 1, sizeof(pathbuf) - (sizeof(BASE_PATH) - 1), "%s.lua", sbuf_str(cur->path));
#undef BASE_PATH

        if (fs_modtime(pathbuf, &mtime))
        {
            /* Path no longer exists, remove item */
            if (bit_is_pow2_or_zero(delCount))
            {
                uint32_t cap = (delCount == 0) ? 1 : delCount * 2;
                uint32_t* dels = realloc_array_type(delItemIds, cap, uint32_t);

                if (!dels)
                {
                    rc = ERR_OutOfMemory;
                    goto fail;
                }

                delItemIds = dels;
            }

            delItemIds[delCount++] = cur->itemId;
            cur->proto = item_proto_destroy(cur->proto);
            continue;
        }

        if (cur->modTime < mtime)
        {
            /* Item file has been edited */
            lua_pushlstring(L, sbuf_str(cur->path), sbuf_length(cur->path));
            lua_pushinteger(L, (int)cur->itemId);
            lua_settable(L, -3);
            replaceCount++;
            cur->proto = item_proto_destroy(cur->proto);
            continue;
        }

        /* Use the item proto that was cached in the DB */
        rc = item_proto_add_from_db(itemList, cur->proto, cur->itemId);
        if (rc) goto fail;

        /* Add to the skip list for lua */
        lua_pushlstring(L, sbuf_str(cur->path), sbuf_length(cur->path));
        lua_pushboolean(L, 1);
        lua_settable(L, -3);

        useCount++;
    }

    log_writef(logQueue, logId, "Loaded items from database: %u cached, %u out of date, %u removed", useCount, replaceCount, delCount);

    lua_setglobal(L, "gSkipPaths");
    lua_pushlightuserdata(L, itemList);
    lua_setglobal(L, "gItemList");
    lua_pushlightuserdata(L, logQueue);
    lua_setglobal(L, "gLogQueue");
    lua_pushinteger(L, logId);
    lua_setglobal(L, "gLogId");
    lua_pushlightuserdata(L, &changes);
    lua_setglobal(L, "gChanges");

    time = clock_milliseconds();
    rc = zlua_script(L, "script/sys/load_items.lua", 1, logQueue, logId);

    if (rc == ERR_None)
    {
        time = clock_milliseconds() - time;
        log_writef(logQueue, logId, "Parsed %i items in %llu.%03llu seconds", lua_tointeger(L, -1), time / 1000ULL, time % 1000ULL);
        lua_pop(L, -1);
    }

    lua_pushnil(L);
    lua_setglobal(L, "gSkipPaths");
    lua_pushnil(L);
    lua_setglobal(L, "gItemList");
    lua_pushnil(L);
    lua_setglobal(L, "gLogQueue");
    lua_pushnil(L);
    lua_setglobal(L, "gLogId");
    lua_pushnil(L);
    lua_setglobal(L, "gChanges");

    /* Send changes to DB */
    if (changes.count > 0)
    {
        query.db.zQuery.qMainItemProtoChanges = changes;
        rc = db_queue_query(mt_get_db_queue(mt), mt_get_queue(mt), mt_get_db_id(mt), 0, ZOP_DB_QueryMainItemProtoChanges, &query);

        if (rc) goto fail_changes;
    }
    else
    {
        *progress |= nth_bit(MAIN_LOAD_ItemProtoChanges);
    }

    if (delCount > 0)
    {
        query.db.zQuery.qMainItemProtoDeletes.count = delCount;
        query.db.zQuery.qMainItemProtoDeletes.itemIds = delItemIds;
        rc = db_queue_query(mt_get_db_queue(mt), mt_get_queue(mt), mt_get_db_id(mt), 0, ZOP_DB_QueryMainItemProtoDeletes, &query);

        if (rc) goto fail_deletes;
    }
    else
    {
        *progress |= nth_bit(MAIN_LOAD_ItemProtoDeletes);
    }

    return rc;

fail:
    lua_pop(L, 1); /* Pop the table we created in this call */
fail_changes:
    free_if_exists(changes.proto);
fail_deletes:
    free_if_exists(delItemIds);
fail_db:
    return rc;
}

static int mt_load_handle_db_result(MainThread* mt, ZPacket* zpacket, uint32_t* progress, uint32_t progBit)
{
    int rc = mt_load_check_db_error(mt, zpacket, FUNC_NAME);

    if (rc == ERR_None)
        *progress |= nth_bit(progBit);

    return rc;
}

static int mt_process_loads(MainThread* mt, RingBuf* mainQueue, uint32_t* progress)
{
    ZPacket zpacket;
    int zop;
    int rc = ERR_None;

    for (;;)
    {
        if (ringbuf_pop(mainQueue, &zop, &zpacket))
            break;

        switch (zop)
        {
        case ZOP_DB_QueryMainLoadItemProtos:
            rc = mt_load_item_protos(mt, &zpacket, progress);
            break;
        
        case ZOP_DB_QueryMainItemProtoChanges:
            rc = mt_load_handle_db_result(mt, &zpacket, progress, MAIN_LOAD_ItemProtoChanges);
            break;

        case ZOP_DB_QueryMainItemProtoDeletes:
            rc = mt_load_handle_db_result(mt, &zpacket, progress, MAIN_LOAD_ItemProtoDeletes);
            break;
        
        case ZOP_MAIN_TerminateAll:
            fprintf(stderr, "Received ZOP_MAIN_TerminateAll signal, aborting mt_process_loads\n");
            return ERR_Cancel;

        default:
            break;
        }

        if (rc) break;
    }

    return rc;
}

int mt_load_all(MainThread* mt)
{
    ZPacket query;
    RingBuf* mainQueue = mt_get_queue(mt);
    uint32_t progress = 0;
    int rc;

    /* Load item protos from DB */
    rc = db_queue_query(mt_get_db_queue(mt), mainQueue, mt_get_db_id(mt), 0, ZOP_DB_QueryMainLoadItemProtos, &query);
    if (rc) goto finish;

    for (;;)
    {
        rc = mt_process_loads(mt, mainQueue, &progress);

        if (rc || (progress ^ bit_mask(MAIN_LOAD_COUNT)) == 0)
            goto finish;

        clock_sleep(25);
    }

finish:
    return rc;
}
