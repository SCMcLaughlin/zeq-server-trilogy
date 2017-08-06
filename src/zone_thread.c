
#include "zone_thread.h"
#include "define_netcode.h"
#include "timer.h"
#include "util_alloc.h"
#include "util_clock.h"
#include "util_thread.h"
#include "zone.h"
#include "zpacket.h"
#include "enum_zop.h"
#include "enum2str.h"

struct ZoneThread {
    int         dbId;
    int         logId;
    RingBuf*    ztQueue;
    RingBuf*    mainQueue;
    RingBuf*    udpQueue;
    RingBuf*    dbQueue;
    RingBuf*    logQueue;
    TimerPool   timerPool;
};

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
    
    zt->dbId = dbId;
    zt->ztQueue = NULL;
    zt->mainQueue = mainQueue;
    zt->udpQueue = udp_get_queue(udp);
    zt->dbQueue = dbQueue;
    zt->logQueue = log_get_queue(log);
    
    rc = log_open_filef(log, &zt->logId, "log/zone_thread_%u_port_%u", index, port);
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
