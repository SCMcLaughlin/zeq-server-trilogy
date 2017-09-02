
#include "zone_thread.h"
#include "aligned.h"
#include "bit.h"
#include "client_packet_recv.h"
#include "define_netcode.h"
#include "log_thread.h"
#include "packet_static.h"
#include "packet_struct.h"
#include "timer.h"
#include "util_alloc.h"
#include "util_clock.h"
#include "util_hash_tbl.h"
#include "util_lua.h"
#include "util_str.h"
#include "util_thread.h"
#include "zone.h"
#include "zone_id.h"
#include "zpacket.h"
#include "enum_opcode.h"
#include "enum_zop.h"
#include "enum2str.h"

#define ZONE_THREAD_EXPECTED_CLIENT_TIMEOUT_MS 15000
#define ZONE_THREAD_UNCONFIRMED_CLIENT_TIMEOUT_MS 15000
#define ZONE_THREAD_CHECK_UNCONFIRMED_TIMEOUTS_MS 5000

typedef struct {
    int16_t zoneId;
    int16_t instId;
} ZoneId;

typedef struct {
    uint32_t        ip;
    StaticBuffer*   name;
} ClientExpectedIpName;

typedef struct {
    uint64_t    timeoutTimestamp;
    Client*     client;
    int16_t     zoneId;
    int16_t     instId;
} ClientExpected;

typedef struct {
    IpAddr  ipAddr;
} ClientStub;

typedef struct {
    uint64_t    timeoutTimestamp;
    ClientStub* clientStub;
} ClientUnconfirmed;

/*
    There is a window of time where we may have to deal with packets associated with a ClientStub while we're in the process
    of replacing that ClientStub with the correct Client on the UdpThread-side. This structs maps ClientStubs to Clients for
    that brief period.
*/
typedef struct {
    ClientStub* stub;
    Client*     client;
} ClientTransitional;

struct ZoneThread {
    uint32_t                clientCount;
    uint32_t                npcCount;
    uint32_t                clientTransitionalCount;
    uint32_t                clientExpectedCount;
    uint32_t                clientUnconfirmedCount;
    uint32_t                zoneCount;
    int                     dbId;
    int                     logId;
    Zone**                  zones;
    Client**                clients;
    ZoneId*                 zoneIds;
    HashTbl                 tblValidClientPtrs;
    ClientTransitional*     clientsTransitional;
    ClientExpectedIpName*   clientsExpectedLookup;
    ClientExpected*         clientsExpected;
    ClientExpectedIpName*   clientsUnconfirmedLookup;
    ClientUnconfirmed*      clientsUnconfirmed;
    lua_State*              lua;
    RingBuf*                ztQueue;
    RingBuf*                mainQueue;
    RingBuf*                udpQueue;
    RingBuf*                dbQueue;
    RingBuf*                logQueue;
    LogThread*              logThread;
    TimerPool               timerPool;
    Timer*                  timerUnconfirmedTimeouts;
    StaticPackets*          staticPackets;
};

static void zt_handle_create_zone(ZoneThread* zt, ZPacket* zpacket)
{
    uint32_t index = zt->zoneCount;
    Zone* zone;
    int zoneId = zpacket->zone.zCreateZone.zoneId;
    int instId = zpacket->zone.zCreateZone.instId;
    
    if (bit_is_pow2_or_zero(index))
    {
        uint32_t cap = (index == 0) ? 1 : index * 2;
        Zone** zones;
        ZoneId* zoneIds;
        
        zones = realloc_array_type(zt->zones, cap, Zone*);
        if (!zones) goto fail;
        zt->zones = zones;
        
        zoneIds = realloc_array_type(zt->zoneIds, cap, ZoneId);
        if (!zoneIds) goto fail;
        zt->zoneIds = zoneIds;
    }
    
    zone = zone_create(zt->logThread, zt->udpQueue, zt->dbQueue, zt->dbId, zoneId, instId, zt->staticPackets, zt->lua);
    if (!zone) goto fail;
    
    zt->zones[index] = zone;
    zt->zoneIds[index].zoneId = (int16_t)zoneId;
    zt->zoneIds[index].instId = (int16_t)instId;
    
    log_writef(zt->logQueue, zt->logId, "Started Zone %i (%s), instance %i", zoneId, zone_short_name_by_id(zoneId), instId);
    zt->zoneCount = index + 1;
    return;
    
fail:
    log_writef(zt->logQueue, zt->logId, "ERROR: zt_handle_create_zone: failure while allocating zone %i inst %i", zoneId, instId);
    /*fixme: tell ZoneMgr this zone has gone down?*/
}

static Zone* zt_get_zone_by_id(ZoneThread* zt, int zoneId, int instId)
{
    ZoneId* zoneIds = zt->zoneIds;
    uint32_t n = zt->zoneCount;
    uint32_t i;
    
    for (i = 0; i < n; i++)
    {
        ZoneId* id = &zoneIds[i];
        
        if (id->zoneId == zoneId && id->instId == instId)
        {
            return zt->zones[i];
        }
    }
    
    return NULL;
}

static void zt_transition_stub(ZoneThread* zt, ClientStub* stub, ClientExpected* expected)
{
    Client* client = expected->client;
    ZPacket zpacket;
    uint32_t index;
    Zone* zone;
    uint32_t n;
    int rc;

    log_writef(zt->logQueue, zt->logId, "Found matching auth for client logging in as \"%s\"", sbuf_str(client_name(client)));

    /* Update the Client's ip address (it is stale) */
    client_set_ip_addr(client, stub->ipAddr);
    
    /* Tell UdpThread to switch its clientObject from the stub to the real Client */
    zpacket.udp.zReplaceClientObject.ipAddress = stub->ipAddr;
    zpacket.udp.zReplaceClientObject.clientObject = client;
    
    rc = ringbuf_push(zt->udpQueue, ZOP_UDP_ReplaceClientObject, &zpacket);
    if (rc) goto fail;
    
    /* Map the stub to the real client while we wait for that to happen */
    index = zt->clientTransitionalCount;
    
    if (bit_is_pow2_or_zero(index))
    {
        uint32_t cap = (index == 0) ? 1 : index * 2;
        ClientTransitional* trans = realloc_array_type(zt->clientsTransitional, cap, ClientTransitional);
        
        if (!trans) goto fail;
        
        zt->clientsTransitional = trans;
    }
    
    zt->clientsTransitional[index].stub = stub;
    zt->clientsTransitional[index].client = client;
    
    /* Add the Client to the main client list */
    rc = tbl_set_ptr(&zt->tblValidClientPtrs, client, NULL);
    if (rc) goto fail;
    
    index = zt->clientCount;
    
    if (bit_is_pow2_or_zero(index))
    {
        uint32_t cap = (index == 0) ? 1 : index * 2;
        Client** clients = realloc_array_type(zt->clients, cap, Client*);
        
        if (!clients) goto fail;
        
        zt->clients = clients;
    }
    
    zt->clients[index] = client;
    
    /* Inform the target Zone about this Client */
    zone = zt_get_zone_by_id(zt, expected->zoneId, expected->instId);
    if (!zone) goto fail;
    
    zone_add_client_zoning_in(zone, client);
    
    /* Remove the stub from the unconfirmed client list */
    n = zt->clientUnconfirmedCount;
    
    if (n)
    {
        ClientUnconfirmed* unconfirmed = zt->clientsUnconfirmed;
        
        for (index = 0; index < n; index++)
        {
            if (unconfirmed[index].clientStub == stub)
            {
                ClientExpectedIpName* lookup = &zt->clientsUnconfirmedLookup[index];
                
                lookup->name = sbuf_drop(lookup->name);
                
                /* Swap and pop */
                n--;
                unconfirmed[index] = unconfirmed[n];
                *lookup = zt->clientsUnconfirmedLookup[n];
                zt->clientUnconfirmedCount = n;
                break;
            }
        }
    }
    
fail:;
}

static void zt_handle_add_client_expected(ZoneThread* zt, ZPacket* zpacket)
{
    Client* client = zpacket->zone.zAddClientExpected.client;
    int zoneId = zpacket->zone.zAddClientExpected.zoneId;
    int instId = zpacket->zone.zAddClientExpected.instId;
    StaticBuffer* name = client_name(client);
    uint32_t ip = client_ip(client);
    uint32_t n = zt->clientUnconfirmedCount;
    
    /* Check if we already have an unconfirmed client waiting for this (very unlikely) */
    if (n)
    {
        ClientExpectedIpName* unconfirmed = zt->clientsUnconfirmedLookup;
        uint32_t len = sbuf_length(name);
        const char* str = sbuf_str(name);
        uint32_t i;
        
        for (i = 0; i < n; i++)
        {
            ClientExpectedIpName* unconf = &unconfirmed[i];
            
            if (unconf->name && unconf->ip == ip && len == sbuf_length(unconf->name) && strcmp(str, sbuf_str(unconf->name)) == 0)
            {
                ClientExpected expected;
                
                expected.client = client;
                expected.zoneId = (int16_t)zoneId;
                expected.instId = (int16_t)instId;
                
                zt_transition_stub(zt, zt->clientsUnconfirmed[i].clientStub, &expected);
                return;
            }
        }
    }
    
    /* Add to our list of expected clients */
    n = zt->clientExpectedCount;
    
    if (bit_is_pow2_or_zero(n))
    {
        uint32_t cap = (n == 0) ? 1 : n * 2;
        ClientExpectedIpName* lookup;
        ClientExpected* expected;
        
        lookup = realloc_array_type(zt->clientsExpectedLookup, cap, ClientExpectedIpName);
        if (!lookup) goto fail;
        zt->clientsExpectedLookup = lookup;
        
        expected = realloc_array_type(zt->clientsExpected, cap, ClientExpected);
        if (!expected) goto fail;
        zt->clientsExpected = expected;
    }
    
    zt->clientsExpectedLookup[n].ip = ip;
    zt->clientsExpectedLookup[n].name = name;
    
    zt->clientsExpected[n].timeoutTimestamp = clock_milliseconds() + ZONE_THREAD_EXPECTED_CLIENT_TIMEOUT_MS;
    zt->clientsExpected[n].client = client;
    zt->clientsExpected[n].zoneId = (int16_t)zoneId;
    zt->clientsExpected[n].instId = (int16_t)instId;
    
    zt->clientExpectedCount = n + 1;

    log_writef(zt->logQueue, zt->logId, "Authorized %u.%u.%u.%u to log into zone %i (%s) inst %i on character \"%s\", awaiting matching client connection",
        (ip >> 0) & 0xff, (ip >> 8) & 0xff, (ip >> 16) & 0xff, (ip >> 24) & 0xff, zoneId, zone_short_name_by_id(zoneId), instId, sbuf_str(name));
    return;
    
fail:
    log_writef(zt->logQueue, zt->logId, "ERROR: zt_handle_add_client_expected: out of memory while allocating expected client from %u.%u.%u.%u on character \"%s\"",
        (ip >> 0) & 0xff, (ip >> 8) & 0xff, (ip >> 16) & 0xff, (ip >> 24) & 0xff, sbuf_str(name));
    /*fixme: inform ClientMgr?*/
}

static bool zt_drop_client_ip(ZoneThread* zt, IpAddr ipAddr)
{
    ZPacket cmd;
    int rc;
    
    cmd.udp.zDropClient.ipAddress = ipAddr;
    cmd.udp.zDropClient.packet = NULL;
    
    rc = ringbuf_push(zt->udpQueue, ZOP_UDP_DropClient, &cmd);
    
    if (rc)
    {
        uint32_t ip = cmd.udp.zDropClient.ipAddress.ip;
        
        log_writef(zt->logQueue, zt->logId, "WARNING: zt_drop_client_ip: failed to inform UdpThread to drop client from %u.%u.%u.%u:%u, keeping client alive for now",
            (ip >> 0) & 0xff, (ip >> 8) & 0xff, (ip >> 16) & 0xff, (ip >> 24) & 0xff, cmd.udp.zDropClient.ipAddress.port);
    
        return false;
    }
    
    return true;
}

static void zt_handle_add_client_unconfirmed(ZoneThread* zt, ZPacket* zpacket)
{
    ClientStub* stub = (ClientStub*)zpacket->udp.zNewClient.clientObject;
    uint32_t index = zt->clientUnconfirmedCount;
    
    stub->ipAddr = zpacket->udp.zNewClient.ipAddress;
    
    /*
        We have to wait for the client to send us the name of their character before we can confirm we are expecting them.
        For now, just add them to the list of unconfirmed clients.
    */
    
    if (bit_is_pow2_or_zero(index))
    {
        uint32_t cap = (index == 0) ? 1 : index * 2;
        ClientExpectedIpName* lookup;
        ClientUnconfirmed* unconf;
        
        lookup = realloc_array_type(zt->clientsUnconfirmedLookup, cap, ClientExpectedIpName);
        if (!lookup) goto fail;
        zt->clientsUnconfirmedLookup = lookup;
        
        unconf = realloc_array_type(zt->clientsUnconfirmed, cap, ClientUnconfirmed);
        if (!unconf) goto fail;
        zt->clientsUnconfirmed = unconf;
    }
    
    zt->clientsUnconfirmedLookup[index].ip = stub->ipAddr.ip;
    zt->clientsUnconfirmedLookup[index].name = NULL;
    
    zt->clientsUnconfirmed[index].timeoutTimestamp = clock_milliseconds() + ZONE_THREAD_UNCONFIRMED_CLIENT_TIMEOUT_MS;
    zt->clientsUnconfirmed[index].clientStub = stub;
    
    zt->clientUnconfirmedCount = index + 1;
    return;

fail:
    zt_drop_client_ip(zt, stub->ipAddr);
}

static void zt_handle_unconfirmed_client_name(ZoneThread* zt, ClientStub* stub, const char* name)
{
    uint32_t len = (uint32_t)str_len_bounded(name, sizeof_field(PS_ZoneEntryClient, name));
    uint32_t ip = stub->ipAddr.ip;
    ClientExpectedIpName* lookup = zt->clientsExpectedLookup;
    ClientUnconfirmed* unconfirmed;
    uint32_t n = zt->clientExpectedCount;
    uint32_t i;

    log_writef(zt->logQueue, zt->logId, "Client from %u.%u.%u.%u attempting to log in as \"%*s\"",
        (ip >> 0) & 0xff, (ip >> 8) & 0xff, (ip >> 16) & 0xff, (ip >> 24) & 0xff, len, name);

    
    /* Check if we now match one of the expected clients */
    for (i = 0; i < n; i++)
    {
        ClientExpectedIpName* check = &lookup[i];
        
        if (check->ip == ip && len == sbuf_length(check->name) && strncmp(name, sbuf_str(check->name), len) == 0)
        {
            zt_transition_stub(zt, stub, &zt->clientsExpected[i]);
            
            /* Swap and pop */
            n--;
            lookup[i] = lookup[n];
            zt->clientsExpected[i] = zt->clientsExpected[n];
            zt->clientExpectedCount = n;
            return;
        }
    }
    
    /* No match, add this name to our unconfirmed client entry */
    unconfirmed = zt->clientsUnconfirmed;
    n = zt->clientUnconfirmedCount;
    
    for (i = 0; i < n; i++)
    {
        if (unconfirmed[i].clientStub == stub)
        {
            /* This could fail... in which case we can just wait for them to be timed out */
            StaticBuffer* sbuf = sbuf_create(name, len);
            
            zt->clientsUnconfirmedLookup[i].name = sbuf;
            sbuf_grab(sbuf);
            break;
        }
    }
}

static void zt_handle_op_zone_entry_stub(ZoneThread* zt, ZPacket* zpacket, ClientStub* stub)
{
    Aligned a;
    const char* name;
    
    aligned_init(&a, zpacket->udp.zToServerPacket.data, zpacket->udp.zToServerPacket.length);
    
    if (aligned_remaining_data(&a) < sizeof(PS_ZoneEntryClient))
        return;
    
    /* PS_ZoneEntryClient */
    /* checksum */
    aligned_advance_by_sizeof(&a, uint32_t);
    /* name */
    name = aligned_current_type(&a, const char);
    
    zt_handle_unconfirmed_client_name(zt, stub, name);
}

static void zt_handle_to_server_packet_stub(ZoneThread* zt, ZPacket* zpacket, ClientStub* stub)
{
    uint16_t opcode = zpacket->udp.zToServerPacket.opcode;
    byte* data = zpacket->udp.zToServerPacket.data;
    
    switch (opcode)
    {
    case OP_SetDataRate:
        break;
    
    case OP_ZoneEntry:
        zt_handle_op_zone_entry_stub(zt, zpacket, stub);
        break;
    
    default:
        log_writef(zt->logQueue, zt->logId, "WARNING: zt_handle_server_packet_stub: received unexpected or unhandled opcode: 0x%04x (%s)",
            opcode, enum2str_opcode(opcode));
        break;
    }
    
    if (data) free(data);
}

static Client* zt_get_client_transitional(ZoneThread* zt, ClientStub* stub)
{
    ClientTransitional* transitional = zt->clientsTransitional;
    uint32_t n = zt->clientTransitionalCount;
    uint32_t i;
    
    for (i = 0; i < n; i++)
    {
        ClientTransitional* trans = &transitional[i];
        
        if (trans->stub == stub)
        {
            return trans->client;
        }
    }
    
    return NULL;
}

static void zt_handle_to_server_packet(ZoneThread* zt, ZPacket* zpacket)
{
    void* clientObject = zpacket->udp.zToServerPacket.clientObject;
    ClientStub* stub;
    Client* client;
    
    /* Is clientObject a known, active Client? */
    if (tbl_has_ptr(&zt->tblValidClientPtrs, clientObject))
    {
        client_packet_recv((Client*)clientObject, zpacket);
        return;
    }
    
    /* Is this a transitional ClientStub which is already mapped to a Client? */
    stub = (ClientStub*)clientObject;
    client = zt_get_client_transitional(zt, stub);
    
    if (client)
    {
        client_packet_recv(client, zpacket);
        return;
    }
    
    /* Must be an unconfirmed ClientStub, handle their packets separately */
    zt_handle_to_server_packet_stub(zt, zpacket, stub);
}

static void zt_remove_transitional(ZoneThread* zt, ClientStub* stub)
{
    ClientTransitional* transitional = zt->clientsTransitional;
    uint32_t n = zt->clientTransitionalCount;
    uint32_t i;
    
    /* Remove from the transitional mapping */
    for (i = 0; i < n; i++)
    {
        if (transitional[i].stub == stub)
        {
            /* Swap and pop */
            n--;
            transitional[i] = transitional[n];
            zt->clientTransitionalCount = n;
            break;
        }
    }
    
    free(stub);
}

static void zt_handle_unconfirmed_client_disconnect(ZoneThread* zt, ClientStub* stub)
{
    ClientUnconfirmed* unconfirmed = zt->clientsUnconfirmed;
    uint32_t n = zt->clientUnconfirmedCount;
    uint32_t i;
    
    for (i = 0; i < n; i++)
    {
        if (unconfirmed[i].clientStub == stub)
        {
            ClientExpectedIpName* lookup = &zt->clientsUnconfirmedLookup[i];
            
            lookup->name = sbuf_drop(lookup->name);
            
            /* Swap and pop */
            n--;
            *lookup = zt->clientsUnconfirmedLookup[n];
            unconfirmed[i] = unconfirmed[n];
            zt->clientUnconfirmedCount = n;
            break;
        }
    }
    
    free(stub);
}

static void zt_handle_client_disconnect(ZoneThread* zt, ZPacket* zpacket)
{
    void* clientObject = zpacket->udp.zClientDisconnect.clientObject;
    ClientStub* stub;
    Client* client;
    
    /* Is clientObject a known, active Client? */
    if (tbl_has_ptr(&zt->tblValidClientPtrs, clientObject))
    {
        /*fixme: handle*/
        return;
    }
    
    /* Is this a transitional ClientStub which is already mapped to a Client? */
    stub = (ClientStub*)clientObject;
    client = zt_get_client_transitional(zt, stub);
    
    if (client)
    {
        zt_remove_transitional(zt, stub);
        /*fixme: handle the Client*/
        return;
    }
    
    /* Must be an unconfirmed ClientStub */
    zt_handle_unconfirmed_client_disconnect(zt, stub);
}

static bool zt_process_commands(ZoneThread* zt, RingBuf* ztQueue)
{
    ZPacket zpacket;
    int zop;
    int rc;
    
    for (;;)
    {
        rc = ringbuf_pop(ztQueue, &zop, &zpacket);
        if (rc) break;
        
        switch (zop)
        {
        case ZOP_UDP_NewClient:
            zt_handle_add_client_unconfirmed(zt, &zpacket);
            break;
        
        case ZOP_UDP_ToServerPacket:
            zt_handle_to_server_packet(zt, &zpacket);
            break;
        
        case ZOP_UDP_ClientDisconnect:
            zt_handle_client_disconnect(zt, &zpacket);
            break;
        
        case ZOP_UDP_ClientLinkdead:
            break;
        
        case ZOP_UDP_ReplaceClientObject:
            zt_remove_transitional(zt, (ClientStub*)zpacket.udp.zReplaceClientObject.clientObject);
            break;
        
        case ZOP_ZONE_TerminateThread:
            return false;
        
        case ZOP_ZONE_CreateZone:
            zt_handle_create_zone(zt, &zpacket);
            break;
        
        case ZOP_ZONE_AddClientExpected:
            zt_handle_add_client_expected(zt, &zpacket);
            break;
        
        default:
            log_writef(zt->logQueue, zt->logId, "WARNING: zt_process_commands: received unexpected zop: %s", enum2str_zop(zop));
            break;
        }
    }
    
    return true;
}

static void zt_thread_on_terminate(ZoneThread* zt)
{
    ZPacket zpacket;
    
    zpacket.zone.zShutDownZoneThread.zt = zt;
    ringbuf_push(zt->mainQueue, ZOP_ZONE_TerminateThread, &zpacket);
}

static void zt_thread_proc(void* ptr)
{
    ZoneThread* zt = (ZoneThread*)ptr;
    RingBuf* ztQueue = zt->ztQueue;
    
    for (;;)
    {
        if (!zt_process_commands(zt, ztQueue))
            break;
        
        timer_pool_execute_callbacks(&zt->timerPool);
        
        clock_sleep(25);
    }
    
    zt_thread_on_terminate(zt);
}

static void zt_timer_check_unconfirmed_timeouts(TimerPool* pool, Timer* timer)
{
    ZoneThread* zt = timer_userdata_type(timer, ZoneThread);
    ClientExpected* expected = zt->clientsExpected;
    ClientUnconfirmed* unconfirmed;
    uint64_t time = clock_milliseconds();
    uint32_t n = zt->clientExpectedCount;
    uint32_t i = 0;
    
    (void)pool;
    
    while (i < n)
    {
        ClientExpected* cur = &expected[i];
        
        if (cur->timeoutTimestamp <= time)
        {
            /*fixme: inform the ClientMgr*/
            
            /* Swap and pop */
            n--;
            expected[i] = expected[n];
            zt->clientsExpectedLookup[i] = zt->clientsExpectedLookup[n];
            continue;
        }
        
        i++;
    }
    
    zt->clientExpectedCount = n;
    
    unconfirmed = zt->clientsUnconfirmed;
    n = zt->clientUnconfirmedCount;
    i = 0;
    
    while (i < n)
    {
        ClientUnconfirmed* cur = &unconfirmed[i];
        
        if (cur->timeoutTimestamp <= time)
        {
            ClientExpectedIpName* lookup = &zt->clientsUnconfirmedLookup[i];
            
            lookup->name = sbuf_drop(lookup->name);
            
            /* The ClientStub itself will be freed when the UdpThread fires ClientDisconnect */
            zt_drop_client_ip(zt, cur->clientStub->ipAddr);
            
            /* Swap and pop */
            n--;
            *lookup = zt->clientsUnconfirmedLookup[n];
            unconfirmed[i] = unconfirmed[n];
            continue;
        }
        
        i++;
    }
    
    zt->clientUnconfirmedCount = n;
}

ZoneThread* zt_create(RingBuf* mainQueue, LogThread* log, RingBuf* dbQueue, int dbId, UdpThread* udp, uint32_t index, uint16_t port, StaticPackets* staticPackets)
{
    ZoneThread* zt = alloc_type(ZoneThread);
    int rc;
    
    if (!zt) goto fail_alloc;
    
    memset(zt, 0, sizeof(ZoneThread));
    
    timer_pool_init(&zt->timerPool);
    
    zt->dbId = dbId;
    tbl_init_size(&zt->tblValidClientPtrs, 0);
    zt->mainQueue = mainQueue;
    zt->udpQueue = udp_get_queue(udp);
    zt->dbQueue = dbQueue;
    zt->logQueue = log_get_queue(log);
    zt->logThread = log;
    zt->staticPackets = staticPackets;
    
    rc = log_open_filef(log, &zt->logId, "log/zone_thread_%u_port_%u.txt", index, port);
    if (rc) goto fail;

    zt->ztQueue = ringbuf_create_type(ZPacket, 1024);
    if (!zt->ztQueue) goto fail;
    
    rc = udp_open_port(udp, port, sizeof(ClientUnconfirmed), zt->ztQueue);
    if (rc) goto fail;
    
    zt->lua = zlua_create(zt->ztQueue, zt->logId);
    if (!zt->lua) goto fail;
    
    rc = zlua_init_zone_thread(zt->lua, zt);
    if (rc) goto fail;
    
    zt->timerUnconfirmedTimeouts = timer_new(&zt->timerPool, ZONE_THREAD_CHECK_UNCONFIRMED_TIMEOUTS_MS, zt_timer_check_unconfirmed_timeouts, zt, true);

    rc = thread_start(zt_thread_proc, zt);
    if (rc) goto fail;
    
    return zt;
    
fail:
    zt_destroy(zt);
fail_alloc:
    return NULL;
}

static void zt_remove_zone(ZoneThread* zt, Zone* zone)
{
    ZPacket zpacket;
    
    zpacket.zone.zRemoveZone.zoneId = zone_id(zone);
    zpacket.zone.zRemoveZone.instId = zone_inst_id(zone);
    
    ringbuf_push(zt->mainQueue, ZOP_MAIN_RemoveZone, &zpacket);
}

static void zt_free_all_zones(ZoneThread* zt)
{
    Zone** zones = zt->zones;
    
    if (zones)
    {
        uint32_t n = zt->zoneCount;
        uint32_t i;
        
        for (i = 0; i < n; i++)
        {
            Zone* zone = zones[i];
            
            zt_remove_zone(zt, zone);
            zone_destroy(zone);
        }
        
        free(zones);
    }
}

ZoneThread* zt_destroy(ZoneThread* zt)
{
    if (zt)
    {
        zt->ztQueue = ringbuf_destroy(zt->ztQueue);
        
        zt->timerUnconfirmedTimeouts = timer_destroy(&zt->timerPool, zt->timerUnconfirmedTimeouts);
        
        zt_free_all_zones(zt);
        free_if_exists(zt->clients); /* ClientMgr owns the individual Client objects */
        free_if_exists(zt->zoneIds);
        
        tbl_deinit(&zt->tblValidClientPtrs, NULL);
        log_close_file(zt->logQueue, zt->logId);
        zt->lua = zlua_destroy(zt->lua);
        free(zt);
    }
    
    return NULL;
}

RingBuf* zt_get_queue(ZoneThread* zt)
{
    return zt->ztQueue;
}

RingBuf* zt_get_log_queue(ZoneThread* zt)
{
    return zt->logQueue;
}

int zt_get_log_id(ZoneThread* zt)
{
    return zt->logId;
}

TimerPool* zt_timer_pool(ZoneThread* zt)
{
    return &zt->timerPool;
}

lua_State* zt_lua(ZoneThread* zt)
{
    return zt->lua;
}
