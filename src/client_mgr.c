
#include "client_mgr.h"
#include "bit.h"
#include "db_thread.h"
#include "main_thread.h"
#include "util_alloc.h"
#include "enum_zop.h"
#include "enum2str.h"

int cmgr_init(MainThread* mt)
{
    ClientMgr* cmgr = mt_get_cmgr(mt);
    RingBuf* mainQueue = mt_get_queue(mt);
    RingBuf* dbQueue = mt_get_db_queue(mt);
    int dbId = mt_get_db_id(mt);
    ZPacket zpacket;
    int rc;
    
    cmgr->clientCount = 0;
    cmgr->loadingClientCount = 0;
    cmgr->guildCount = 0;
    cmgr->clients = NULL;
    cmgr->loadingClients = NULL;
    tbl_init(&cmgr->clientsByName, Client*);
    cmgr->guilds = NULL;

    rc = db_queue_query(dbQueue, mainQueue, dbId, 0, ZOP_DB_QueryMainGuildList, &zpacket);
    if (rc) goto ret;

ret:
    return rc;
}

void cmgr_deinit(ClientMgr* cmgr)
{
    free_if_exists(cmgr->guilds);
}

void cmgr_handle_guild_list(MainThread* mt, ZPacket* zpacket)
{
    ClientMgr* cmgr = mt_get_cmgr(mt);
    Guild* guilds;
    uint32_t count;

    /* Taking direct ownership of the buffer allocated by the DB thread */
    count = zpacket->db.zResult.rMainGuildList.count;
    guilds = zpacket->db.zResult.rMainGuildList.guilds;

    cmgr->guildCount = count;
    cmgr->guilds = guilds;

    if (guilds && count)
    {
        RingBuf* csQueue = mt_get_cs_queue(mt);
        ZPacket toCS;
        uint32_t i;
        int rc;

        count--;

        for (i = 0; i <= count; i++)
        {
            toCS.cs.zAddGuild.isLast = (i == count);
            toCS.cs.zAddGuild.guild = guilds[i];

            sbuf_grab(toCS.cs.zAddGuild.guild.guildName);

            rc = ringbuf_push(csQueue, ZOP_CS_AddGuild, &toCS);

            if (rc)
            {
                sbuf_drop(toCS.cs.zAddGuild.guild.guildName);
                break;
            }
        }
    }
}

static void cmgr_send_char_select_zone_unavailable(MainThread* mt, void* client)
{
    ZPacket zpacket;
    int rc;

    zpacket.cs.zZoneFailure.client = client;
    rc = ringbuf_push(mt_get_cs_queue(mt), ZOP_CS_ZoneFailure, &zpacket);

    if (rc)
        log_writef(mt_get_log_queue(mt), mt_get_log_id(mt), "ERROR: cmgr_send_char_select_zone_unavailable: error %s", enum2str_err(rc));
}

void cmgr_handle_zone_from_char_select(MainThread* mt, ZPacket* zpacket)
{
    ClientMgr* cmgr = mt_get_cmgr(mt);
    void* csClient = zpacket->main.zZoneFromCharSelect.client;
    StaticBuffer* name = zpacket->main.zZoneFromCharSelect.name;
    int64_t accountId = zpacket->main.zZoneFromCharSelect.accountId;
    int rc = ERR_OutOfMemory;
    Client* client = NULL;

    /* Step 1: check if we already have a client by this name logged in */
    /*fixme: decide how to handle this re: the existing client in a zone, if any*/

    /* Step 2: create a new, unloaded client */
    client = client_create_unloaded(name, accountId, zpacket->main.zZoneFromCharSelect.isLocal);

    if (client)
    {
        uint32_t index = cmgr->clientCount;
        ZPacket query;
        int rc;

        if (bit_is_pow2_or_zero(index))
        {
            uint32_t cap = (index == 0) ? 1 : index * 2;
            ClientLoading* loading = realloc_array_type(cmgr->loadingClients, cap, ClientLoading);

            if (!loading) goto fail;

            cmgr->loadingClients = loading;
        }

        cmgr->loadingClients[index].client = client;
        cmgr->loadingClients[index].csClient = csClient;
        cmgr->loadingClients[index].queriesCompleted = 0;
        cmgr->loadingClientCount = index + 1;

        rc = tbl_set_sbuf(&cmgr->clientsByName, name, (void*)&client);
        if (rc) goto undo_add;

        /* Step 3: run the first DB query to get the character's id and primary stats */
        query.db.zQuery.qMainLoadCharacter.client = client;
        query.db.zQuery.qMainLoadCharacter.accountId = accountId;
        query.db.zQuery.qMainLoadCharacter.name = name;

        rc = db_queue_query(mt_get_db_queue(mt), mt_get_queue(mt), mt_get_db_id(mt), 0, ZOP_DB_QueryMainLoadCharacter, &query);
        if (rc)
        {
            tbl_remove_str(&cmgr->clientsByName, sbuf_str(name), sbuf_length(name));
        undo_add:
            cmgr->loadingClientCount = index;
            goto fail;
        }

        return;
    }

fail:
    /*
        Drop the ref to the name that originally belonged to the CharSelect thread and was transferred to the MainThread.
        If db_queue_query() succeeded, the DB thread now owns this ref instead.
    */
    sbuf_drop(name);

    if (client)
        client_destroy(client);

    log_writef(mt_get_log_queue(mt), mt_get_log_id(mt), "ERROR: cmgr_handle_zone_from_char_select: error %s while allocating new client instance \"%s\"",
        enum2str_err(rc), sbuf_str(name));
    /* Tell the client the zone is unavailable, for the sake of responding in some way */
    cmgr_send_char_select_zone_unavailable(mt, csClient);
}

static bool cmgr_is_loading_client_valid(MainThread* mt, Client* client, ClientLoading** out)
{
    ClientMgr* cmgr = mt_get_cmgr(mt);
    uint32_t count = cmgr->loadingClientCount;
    ClientLoading* loading = cmgr->loadingClients;
    uint32_t i;

    for (i = 0; i < count; i++)
    {
        if (loading[i].client == client)
        {
            *out = &loading[i];
            return true;
        }
    }

    return false;
}

static int cmgr_check_loading_db_error(MainThread* mt, Client* client, ZPacket* zpacket, ClientLoading** out, const char* funcName)
{
    if (zpacket->db.zResult.hadErrorUnprocessed)
    {
        log_writef(mt_get_log_queue(mt), mt_get_log_id(mt), "WARNING: %s: database reported error, query unprocessed", funcName);
        return ERR_Invalid;
    }

    if (!cmgr_is_loading_client_valid(mt, client, out))
    {
        log_writef(mt_get_log_queue(mt), mt_get_log_id(mt), "WARNING: %s: received database result for CharSelectClient that no longer exists", funcName);
        return ERR_Invalid;
    }

    if (zpacket->db.zResult.hadError)
    {
        log_writef(mt_get_log_queue(mt), mt_get_log_id(mt), "WARNING: %s: database reported error", funcName);
        return ERR_Generic;
    }

    return ERR_None;
}

static void cmgr_drop_loading_client(MainThread* mt, Client* client)
{
    ClientMgr* cmgr = mt_get_cmgr(mt);
    uint32_t count = cmgr->loadingClientCount;
    ClientLoading* loading = cmgr->loadingClients;
    uint32_t i;

    for (i = 0; i < count; i++)
    {
        if (loading[i].client == client)
        {
            client_destroy(client);
            cmgr_send_char_select_zone_unavailable(mt, loading[i].csClient);

            /* Swap and pop */
            count--;
            loading[i] = loading[count];
            cmgr->loadingClientCount = count;
            break;
        }
    }
}

void cmgr_handle_load_character(MainThread* mt, ZPacket* zpacket)
{
    ClientMgr* cmgr = mt_get_cmgr(mt);
    Client* client = (Client*)zpacket->db.zResult.rMainLoadCharacter.client;
    ClientLoadData_Character* data = zpacket->db.zResult.rMainLoadCharacter.data;
    ClientLoading* loading;

    switch (cmgr_check_loading_db_error(mt, client, zpacket, &loading, FUNC_NAME))
    {
    case ERR_Invalid:
        return;

    case ERR_Generic:
        goto drop_client;

    case ERR_None:
        break;
    }

    if (!data) goto drop_only;

    loading->queriesCompleted = 1;
    client_load_character_data(client, data);

    /* Do subsequent queries, now that we have the character id to use */
    /*fixme: add these*/

    free(data);
    return;

drop_client:
    if (data)
    {
        data->surname = sbuf_drop(data->surname);
        free(data);
    }
drop_only:
    cmgr_drop_loading_client(mt, client);
}
