
#include "zone.h"
#include "client.h"
#include "ringbuf.h"
#include "util_alloc.h"
#include "zone_id.h"
#include "zone_thread.h"

struct Zone {
    uint32_t    clientCount;
    uint32_t    npcCount;
    int16_t     zoneId;
    int16_t     instId;
    int         logId;
    Client**    clients;
    RingBuf*    logQueue;
};

Zone* zone_create(LogThread* log, int zoneId, int instId)
{
    Zone* zone = alloc_type(Zone);
    int rc;
    
    if (!zone) goto fail_alloc;
    
    zone->zoneId = (int16_t)zoneId;
    zone->instId = (int16_t)instId;
    zone->logQueue = log_get_queue(log);
    
    rc = log_open_filef(log, &zone->logId, "log/zone_%s_inst_%i.txt", zone_short_name_by_id(zoneId), instId);
    if (rc) goto fail;
    
fail:
    zone_destroy(zone);
fail_alloc:
    return NULL;
}

Zone* zone_destroy(Zone* zone)
{
    if (zone)
    {
        log_close_file(zone->logQueue, zone->logId);
        free(zone);
    }
    
    return NULL;
}
