
#include "client_mgr.h"
#include "bit.h"
#include "db_thread.h"
#include "main_thread.h"
#include "util_alloc.h"
#include "zone_mgr.h"
#include "enum_zop.h"
#include "enum2str.h"

#define CMGR_NUM_CLIENT_LOAD_QUERIES 2

int cmgr_init(MainThread* mt)
{
    ClientMgr* cmgr = mt_get_cmgr(mt);
    RingBuf* mainQueue = mt_get_queue(mt);
    RingBuf* dbQueue = mt_get_db_queue(mt);
    int dbId = mt_get_db_id(mt);
    ZPacket zpacket;
    int rc;
    
    /* This struct has already been memset(0)'d */
    
    tbl_init(&cmgr->clientsByName, Client*);

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
    client = client_create_unloaded(name, accountId, zpacket->main.zZoneFromCharSelect.ipAddr, zpacket->main.zZoneFromCharSelect.isLocal);

    if (client)
    {
        RingBuf* dbQueue;
        RingBuf* mainQueue;
        int dbId;
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
        
        dbQueue = mt_get_db_queue(mt);
        mainQueue = mt_get_queue(mt);
        dbId = mt_get_db_id(mt);

        /* Step 3: run character queries */
        query.db.zQuery.qMainLoadCharacter.client = client;
        query.db.zQuery.qMainLoadCharacter.accountId = accountId;
        query.db.zQuery.qMainLoadCharacter.name = name;
        
        /* Character id and main stats query */
        sbuf_grab(name);
        rc = db_queue_query(dbQueue, mainQueue, dbId, 0, ZOP_DB_QueryMainLoadCharacter, &query);
        if (rc) goto fail_db;
        
        /* Inventory query */
        sbuf_grab(name);
        rc = db_queue_query(dbQueue, mainQueue, dbId, 0, ZOP_DB_QueryMainLoadInventory, &query);
        if (rc) goto fail_db;

        /* Drop the ref to the name that originally belonged to the CharSelect thread and was transferred to the MainThread */
        sbuf_drop(name);
        return;
        
    fail_db:
        sbuf_drop(name); /* Drop the ref added for the query that failed db_queue_query() */
        tbl_remove_str(&cmgr->clientsByName, sbuf_str(name), sbuf_length(name));
    undo_add:
        cmgr->loadingClientCount = index;
        goto fail;
    }

fail:
    /* Drop the ref to the name that originally belonged to the CharSelect thread and was transferred to the MainThread */
    sbuf_drop(name);

    /* Destroy the client we just created, which is not yet associated with any zone */
    if (client)
        client_destroy_no_zone(client);

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
        return ERR_NotFound;
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
            client_destroy_no_zone(client);
            cmgr_send_char_select_zone_unavailable(mt, loading[i].csClient);

            /* Swap and pop */
            count--;
            loading[i] = loading[count];
            cmgr->loadingClientCount = count;
            break;
        }
    }
}

static void cmgr_drop_load_data_character(ClientLoadData_Character* data)
{
    if (data)
    {
        data->surname = sbuf_drop(data->surname);
        free(data);
    }
}

static int cmgr_add_client(MainThread* mt, Client* client, ClientLoading* loading)
{
    ClientMgr* cmgr = mt_get_cmgr(mt);
    ClientLoading* loadingClients;
    uint32_t n;
    uint32_t i;
    
    /* Add to main list */
    i = cmgr->clientCount;
    
    if (bit_is_pow2_or_zero(i))
    {
        uint32_t cap = (i == 0) ? 1 : i * 2;
        Client** clients = realloc_array_type(cmgr->clients, cap, Client*);
        
        if (!clients) return ERR_OutOfMemory;
        
        cmgr->clients = clients;
    }
    
    cmgr->clients[i] = client;
    
    /* Pop and swap */
    loadingClients = cmgr->loadingClients;
    n = cmgr->loadingClientCount;
    i = (uint32_t)(loading - loadingClients); /* This gives us the array index */
    
    n--;
    loadingClients[i] = loadingClients[n];
    cmgr->loadingClientCount = n;
    
    return ERR_None;
}

static void cmgr_check_client_load_complete(MainThread* mt, Client* client, ClientLoading* loading)
{
    ZoneMgr* zmgr;
    ZPacket zpacket;
    uint16_t port;
    bool isLocal;
    int rc;
    
    if (loading->queriesCompleted != CMGR_NUM_CLIENT_LOAD_QUERIES)
        return;
    
    /*
        Load completed, things we need to do:
        1) Calculate the client's stats
        2) Hand the Client off to the ZoneMgr and get the port of the zone they are going to in return
        3) Inform the CharSelectThread about the successful zone with the target IP and port
        4) Remove the ClientLoading entry for this Client and put them into the main client list
    */
    
    isLocal = client_is_local(client);
    
    client_calc_stats_all(client);
    
    rc = zmgr_add_client_from_char_select(mt, client, loading->zoneId, loading->instId, &port);
    if (rc) goto drop_client;
    
    /* IT IS NO LONGER SAFE TO DEREFERENCE THE CLIENT PAST THIS POINT */
    
    zmgr = mt_get_zmgr(mt);
    
    zpacket.cs.zZoneSuccess.client = loading->csClient;
    zpacket.cs.zZoneSuccess.zoneId = loading->zoneId;
    zpacket.cs.zZoneSuccess.port = to_network_uint16(port);
    zpacket.cs.zZoneSuccess.ipAddress = (isLocal) ? zmgr_local_ip(zmgr) : zmgr_remote_ip(zmgr);
    zpacket.cs.zZoneSuccess.motd = mt_get_motd(mt);
    
    sbuf_grab(zpacket.cs.zZoneSuccess.ipAddress);
    sbuf_grab(zpacket.cs.zZoneSuccess.motd);
    
    rc = ringbuf_push(mt_get_cs_queue(mt), ZOP_CS_ZoneSuccess, &zpacket);
    if (rc)
    {
        sbuf_drop(zpacket.cs.zZoneSuccess.ipAddress);
        sbuf_drop(zpacket.cs.zZoneSuccess.motd);
        goto drop_client;
    }
    
    rc = cmgr_add_client(mt, client, loading);
    if (rc)
    {
        log_writef(mt_get_log_queue(mt), mt_get_log_id(mt), "ERROR: cmgr_check_client_load_complete: out of memory while adding client to ClientMgr client list");
        goto drop_client;
    }
    
    return;
    
drop_client:
    cmgr_drop_loading_client(mt, client);
}

void cmgr_handle_load_character(MainThread* mt, ZPacket* zpacket)
{
    Client* client = (Client*)zpacket->db.zResult.rMainLoadCharacter.client;
    ClientLoadData_Character* data = zpacket->db.zResult.rMainLoadCharacter.data;
    ClientLoading* loading;

    switch (cmgr_check_loading_db_error(mt, client, zpacket, &loading, FUNC_NAME))
    {
    case ERR_Invalid:
        return;

    case ERR_NotFound:
        goto drop_data;

    case ERR_Generic:
        goto drop_client;

    case ERR_None:
        break;
    }

    if (!data) goto drop_client;

    loading->queriesCompleted++;
    loading->zoneId = (int16_t)data->zoneId;
    loading->instId = (int16_t)data->instId;

    client_load_character_data(client, data);
    cmgr_check_client_load_complete(mt, client, loading);

    free(data);
    return;

drop_client:
    cmgr_drop_loading_client(mt, client);
drop_data:
    cmgr_drop_load_data_character(data);
}

void cmgr_handle_load_inventory(MainThread* mt, ZPacket* zpacket)
{
    Client* client = (Client*)zpacket->db.zResult.rMainLoadInventory.client;
    uint32_t count = zpacket->db.zResult.rMainLoadInventory.count;
    ClientLoadData_Inventory* data = zpacket->db.zResult.rMainLoadInventory.data;
    ClientLoading* loading;
    int rc;
    
    switch (cmgr_check_loading_db_error(mt, client, zpacket, &loading, FUNC_NAME))
    {
    case ERR_Invalid:
        return;

    case ERR_NotFound:
        goto drop_data;

    case ERR_Generic:
        goto drop_client;

    case ERR_None:
        break;
    }

    loading->queriesCompleted++;
    
    if (data)
    {
        rc = inv_from_db(client_inv(client), data, count, mt_get_item_list(mt));
        if (rc) goto drop_client;
    }
    
    cmgr_check_client_load_complete(mt, client, loading);
    goto drop_data;

drop_client:
    cmgr_drop_loading_client(mt, client);
drop_data:
    free_if_exists(data);
}
