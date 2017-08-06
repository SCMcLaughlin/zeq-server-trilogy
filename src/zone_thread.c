
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
} ClientExpected;

struct ZoneThread {
    uint32_t                clientCount;
    uint32_t                npcCount;
    uint32_t                clientExpectedCount;
    uint32_t                zoneCount;
    int                     dbId;
    int                     logId;
    Zone**                  zones;
    Client**                clients;
    ZoneId*                 zoneIds;
    ClientExpectedIpName*   clientsExpectedLookup;
    ClientExpected*         clientsExpected;
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

static void zt_process_commands(ZoneThread* zt, RingBuf* ztQueue)
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
        case ZOP_ZONE_CreateZone:
            zt_handle_create_zone(zt, &zpacket);
            break;
        
        case ZOP_ZONE_AddClient:
            break;
        
        default:
            log_writef(zt->logQueue, zt->logId, "WARNING: zt_process_commands: received unexpected zop: %s", enum2str_zop(zop));
            break;
        }
    }
}

static void zt_thread_proc(void* ptr)
{
    ZoneThread* zt = (ZoneThread*)ptr;
    RingBuf* ztQueue = zt->ztQueue;
    
    for (;;)
    {
        zt_process_commands(zt, ztQueue);
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
    
    rc = udp_open_port(udp, port, sizeof(IpAddr), zt->ztQueue);
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
