
#include "zone.h"
#include "bit.h"
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
    RingBuf*    udpQueue;
    RingBuf*    logQueue;
};

void zone_add_client(Zone* zone, Client* client)
{
    uint32_t index = zone->clientCount;
    
    if (bit_is_pow2_or_zero(index))
    {
        uint32_t cap = (index == 0) ? 1 : index * 2;
        Client** clients = realloc_array_type(zone->clients, cap, Client*);
        
        if (!clients) goto fail;
        
        zone->clients = clients;
    }
    
    zone->clients[index] = client;
    
    client_set_zone(client, zone);
    
    /*fixme: send packets: PP, ZoneEntry, weather*/
    
    /*fixme: do other initialization here */
    
fail:;
}

Zone* zone_create(LogThread* log, RingBuf* udpQueue, int zoneId, int instId)
{
    Zone* zone = alloc_type(Zone);
    int rc;
    
    if (!zone) goto fail_alloc;
    
    zone->clientCount = 0;
    zone->npcCount = 0;
    zone->zoneId = (int16_t)zoneId;
    zone->instId = (int16_t)instId;
    zone->clients = NULL;
    zone->udpQueue = udpQueue;
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

RingBuf* zone_udp_queue(Zone* zone)
{
    return zone->udpQueue;
}

RingBuf* zone_log_queue(Zone* zone)
{
    return zone->logQueue;
}

int zone_log_id(Zone* zone)
{
    return zone->logId;
}
