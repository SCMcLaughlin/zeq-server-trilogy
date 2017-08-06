
#include "zone_thread.h"
#include "bit.h"
#include "define_netcode.h"
#include "log_thread.h"
#include "timer.h"
#include "util_alloc.h"
#include "util_clock.h"
#include "util_thread.h"
#include "zone.h"
#include "zone_id.h"
#include "zpacket.h"
#include "enum_zop.h"
#include "enum2str.h"

#define ZONE_THREAD_EXPECTED_CLIENT_TIMEOUT_MS 15000
#define ZONE_THREAD_UNCONFIRMED_CLIENT_TIMEOUT_MS 15000

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
    IpAddr  ipAddr;
    Client* client;
} StubToClient;

struct ZoneThread {
    uint32_t                clientCount;
    uint32_t                npcCount;
    uint32_t                clientExpectedCount;
    uint32_t                clientUnconfirmedCount;
    uint32_t                zoneCount;
    int                     dbId;
    int                     logId;
    Zone**                  zones;
    Client**                clients;
    ZoneId*                 zoneIds;
    ClientExpectedIpName*   clientsExpectedLookup;
    ClientExpected*         clientsExpected;
    ClientExpectedIpName*   clientsUnconfirmedLookup;
    ClientUnconfirmed*      clientsUnconfirmed;
    RingBuf*                ztQueue;
    RingBuf*                mainQueue;
    RingBuf*                udpQueue;
    RingBuf*                dbQueue;
    RingBuf*                logQueue;
    LogThread*              logThread;
    TimerPool               timerPool;
};

static void zt_handle_create_zone(ZoneThread* zt, ZPacket* zpacket)
{
    uint32_t index = zt->zoneCount;
    Zone* zone;
    int zoneId;
    int instId;
    
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
    
    zoneId = zpacket->zone.zCreateZone.zoneId;
    instId = zpacket->zone.zCreateZone.instId;
    
    zone = zone_create(zt->logThread, zoneId, instId);
    if (!zone) goto fail;
    
    zt->zones[index] = zone;
    zt->zoneIds[index].zoneId = (int16_t)zoneId;
    zt->zoneIds[index].instId = (int16_t)instId;
    
    log_writef(zt->logQueue, zt->logId, "Started Zone %i (%s), instance %i", zoneId, zone_short_name_by_id(zoneId), instId);
    zt->zoneCount = index + 1;
    return;
    
fail:
    ; /*fixme: tell ZoneMgr this zone has gone down?*/
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
                /*fixme: handle this*/
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
            break;
        
        case ZOP_UDP_ClientDisconnect:
            break;
        
        case ZOP_UDP_ClientLinkdead:
            break;
        
        case ZOP_UDP_ReplaceClientObject:
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
}

ZoneThread* zt_create(RingBuf* mainQueue, LogThread* log, RingBuf* dbQueue, int dbId, UdpThread* udp, uint32_t index, uint16_t port)
{
    ZoneThread* zt = alloc_type(ZoneThread);
    int rc;
    
    if (!zt) goto fail_alloc;
    
    timer_pool_init(&zt->timerPool);
    
    zt->clientCount = 0;
    zt->npcCount = 0;
    zt->zoneCount = 0;
    zt->dbId = dbId;
    zt->zones = NULL;
    zt->clients = NULL;
    zt->zoneIds = NULL;
    zt->clientsExpectedLookup = NULL;
    zt->clientsExpected = NULL;
    zt->clientsUnconfirmedLookup = NULL;
    zt->clientsUnconfirmed = NULL;
    zt->ztQueue = NULL;
    zt->mainQueue = mainQueue;
    zt->udpQueue = udp_get_queue(udp);
    zt->dbQueue = dbQueue;
    zt->logQueue = log_get_queue(log);
    zt->logThread = log;
    
    rc = log_open_filef(log, &zt->logId, "log/zone_thread_%u_port_%u.txt", index, port);
    if (rc) goto fail;

    zt->ztQueue = ringbuf_create_type(ZPacket, 1024);
    if (!zt->ztQueue) goto fail;
    
    rc = udp_open_port(udp, port, sizeof(ClientUnconfirmed), zt->ztQueue);
    if (rc) goto fail;

    rc = thread_start(zt_thread_proc, zt);
    if (rc) goto fail;
    
    return zt;
    
fail:
    zt_destroy(zt);
fail_alloc:
    return NULL;
}

ZoneThread* zt_destroy(ZoneThread* zt)
{
    if (zt)
    {
        zt->ztQueue = ringbuf_destroy(zt->ztQueue);
        
        log_close_file(zt->logQueue, zt->logId);
        free(zt);
    }
    
    return NULL;
}

RingBuf* zt_get_queue(ZoneThread* zt)
{
    return zt->ztQueue;
}
