
#include "zone_mgr.h"
#include "bit.h"
#include "main_thread.h"
#include "packet_static.h"
#include "util_alloc.h"
#include "zone_thread.h"
#include "zpacket.h"
#include "enum_zop.h"

#define ZONE_MGR_FIRST_ZONE_THREAD_PORT 7000

static int zmgr_processor_thread_count()
{
#if defined(PLATFORM_WINDOWS)
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    return (int)info.dwNumberOfProcessors;
#elif defined(_SC_NPROCESSORS_ONLN)
    return sysconf(_SC_NPROCESSORS_ONLN);
#else
    return 0;
#endif
}

int zmgr_init(MainThread* mt)
{
    ZoneMgr* zmgr = mt_get_zmgr(mt);
    int threadsAvailable = zmgr_processor_thread_count();
    int rc;
    
    if (threadsAvailable <= 0)
        threadsAvailable = 1;

    zmgr->zoneCount = 0;
    zmgr->maxZoneThreads = (uint8_t)threadsAvailable;
    zmgr->zoneThreadCount = 0;
    zmgr->nextZoneThreadPort = ZONE_MGR_FIRST_ZONE_THREAD_PORT;
    zmgr->zoneThreads = NULL;
    zmgr->ztByZoneId = NULL;
    zmgr->remoteIpAddress = NULL;
    zmgr->localIpAddress = NULL;

    /*fixme: read these from a script/config file*/
    zmgr->remoteIpAddress = sbuf_create_from_literal("127.0.0.1");
    if (!zmgr->remoteIpAddress) goto oom;
    sbuf_grab(zmgr->remoteIpAddress);

    zmgr->localIpAddress = sbuf_create_from_literal("127.0.0.1");
    if (!zmgr->localIpAddress) goto oom;
    sbuf_grab(zmgr->localIpAddress);
    
    rc = static_packets_init(&zmgr->staticPackets);
    if (rc) return rc;

    return ERR_None;
oom:
    return ERR_OutOfMemory;
}

void zmgr_deinit(ZoneMgr* zmgr)
{
    free_if_exists(zmgr->ztByZoneId);
    zmgr->remoteIpAddress = sbuf_drop(zmgr->remoteIpAddress);
    zmgr->localIpAddress = sbuf_drop(zmgr->localIpAddress);
    static_packets_deinit(&zmgr->staticPackets);
}

static ZoneThreadWithQueue* zmgr_add_zt(MainThread* mt, ZoneMgr* zmgr, uint32_t index)
{
    ZoneThreadWithQueue* ret;
    uint16_t port = zmgr->nextZoneThreadPort++;
    
    if (bit_is_pow2_or_zero(index))
    {
        uint32_t cap = (index == 0) ? 1 : index * 2;
        ZoneThreadWithQueue* zts = realloc_array_type(zmgr->zoneThreads, cap, ZoneThreadWithQueue);
        
        if (!zts) goto oom;
        
        zmgr->zoneThreads = zts;
    }
    
    ret = &zmgr->zoneThreads[index];
    ret->zt = zt_create(mt_get_queue(mt), mt_get_log_thread(mt), mt_get_db_queue(mt), mt_get_db_id(mt), mt_get_udp_thread(mt), index, port, &zmgr->staticPackets);
    
    if (!ret->zt) goto oom;
    
    ret->ztQueue = zt_get_queue(ret->zt);
    ret->port = port;
    ret->zoneCount = 0;
    ret->clientCount = 0;
    
    log_writef(mt_get_log_queue(mt), mt_get_log_id(mt), "Started ZoneThread %u using port %u", index, port);
    
    zmgr->zoneThreadCount = index + 1;
    return ret;
    
oom:
    log_writef(mt_get_log_queue(mt), mt_get_log_id(mt), "ERROR: zmgr_add_zt: out of memory while allocating new ZoneThread");
    return NULL;
}

static ZoneThreadWithQueue* zmgr_select_best_zt_for_new_zone(MainThread* mt, ZoneMgr* zmgr)
{
    ZoneThreadWithQueue* zts;
    uint32_t lowScore;
    uint32_t lowIndex;
    uint32_t n = zmgr->zoneThreadCount;
    uint32_t i;
    
    /* Prefer to make a new ZoneThread if we haven't hit our cap yet */
    if (n < zmgr->maxZoneThreads)
    {
        ZoneThreadWithQueue* ret = zmgr_add_zt(mt, zmgr, n);
        if (ret) return ret;
        
        /* If zmgr_add_zt() failed somehow, we can fall back to trying one of the existing ZoneThreads */
    }
    
    if (n == 0) return NULL;
    
    lowScore = UINT_MAX;
    lowIndex = 0;
    zts = zmgr->zoneThreads;
    
    for (i = 0; i < n; i++)
    {
        ZoneThreadWithQueue* zt = &zts[i];
        uint32_t score = zt->zoneCount * 25 + zt->clientCount;
        
        if (score < lowScore)
        {
            lowScore = score;
            lowIndex = i;
        }
    }
    
    return &zts[lowIndex];
}

static ZoneThreadWithQueue* zmgr_get_or_start_zone(MainThread* mt, int zoneId, int instId)
{
    ZoneMgr* zmgr = mt_get_zmgr(mt);
    ZoneThreadByZoneId* ztByZoneId = zmgr->ztByZoneId;
    ZoneThreadWithQueue* zt;
    ZPacket zpacket;
    uint32_t n = zmgr->zoneCount;
    uint32_t i;
    int rc;
    
    /* Check if we have this zone running already */
    for (i = 0; i < n; i++)
    {
        ZoneThreadByZoneId* by = &ztByZoneId[i];
        
        if (by->zoneId == zoneId && by->instId == instId)
        {
            return &zmgr->zoneThreads[by->ztIndex];
        }
    }
    
    /* Select an existing ZoneThread, or start a new one */
    zt = zmgr_select_best_zt_for_new_zone(mt, zmgr);
    if (!zt) return NULL;
    
    /* Map our ZoneThread to this zoneId and instId */
    if (bit_is_pow2_or_zero(n))
    {
        uint32_t cap = (n == 0) ? 1 : n * 2;
        ZoneThreadByZoneId* byId = realloc_array_type(zmgr->ztByZoneId, cap, ZoneThreadByZoneId);
        
        if (!byId) return NULL;
        
        zmgr->ztByZoneId = byId;
    }
    
    ztByZoneId = &zmgr->ztByZoneId[n];
    ztByZoneId->zoneId = (int16_t)zoneId;
    ztByZoneId->instId = (int16_t)instId;
    ztByZoneId->ztIndex = (uint8_t)(zt - zmgr->zoneThreads); /* This gives us the array index */
    
    /* Inform the target ZoneThread it needs to start a Zone with these ids */
    zpacket.zone.zCreateZone.zoneId = zoneId;
    zpacket.zone.zCreateZone.instId = instId;
    
    rc = ringbuf_push(zt->ztQueue, ZOP_ZONE_CreateZone, &zpacket);
    if (rc)
    {
        log_writef(mt_get_log_queue(mt), mt_get_log_id(mt), "ERROR: zmgr_get_or_start_zone: failed to inform ZoneThread %u to start zoneId %i, instId %i",
            ztByZoneId->ztIndex, zoneId, instId);
        return NULL;
    }
    
    zt->zoneCount++;
    zmgr->zoneCount = n + 1;
    return zt;
}

int zmgr_add_client_from_char_select(MainThread* mt, Client* client, int zoneId, int instId, uint16_t* outPort)
{
    ZoneThreadWithQueue* zt = zmgr_get_or_start_zone(mt, zoneId, instId);
    ZPacket zpacket;
    
    if (!zt) return ERR_OutOfMemory;
    
    *outPort = zt->port;
    
    zt->clientCount++;
    
    zpacket.zone.zAddClientExpected.zoneId = zoneId;
    zpacket.zone.zAddClientExpected.instId = instId;
    zpacket.zone.zAddClientExpected.client = client;
    
    return ringbuf_push(zt->ztQueue, ZOP_ZONE_AddClientExpected, &zpacket);
}

StaticBuffer* zmgr_remote_ip(ZoneMgr* zmgr)
{
    return zmgr->remoteIpAddress;
}

StaticBuffer* zmgr_local_ip(ZoneMgr* zmgr)
{
    return zmgr->localIpAddress;
}

