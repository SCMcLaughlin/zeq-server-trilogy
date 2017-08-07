
#include "zone.h"
#include "bit.h"
#include "client.h"
#include "client_packet_send.h"
#include "misc_struct.h"
#include "loc.h"
#include "ringbuf.h"
#include "util_alloc.h"
#include "zone_id.h"
#include "zone_thread.h"

struct Zone {
    uint32_t        clientCount;
    uint32_t        npcCount;
    int16_t         zoneId;
    int16_t         instId;
    uint16_t        skyType;
    uint8_t         zoneType;
    int             weatherType;
    int             weatherIntensity;
    ZoneFog         fog[4];
    float           gravity;
    float           minZ;
    float           minClippingDistance;
    float           maxClippingDistance;
    LocH            safeSpot;
    int             logId;
    Client**        clients;
    RingBuf*        udpQueue;
    RingBuf*        logQueue;
    StaticPackets*  staticPackets;
};

void zone_add_client(Zone* zone, Client* client)
{
    uint32_t index = zone->clientCount;
    
    /*fixme: don't put the client into the main list until it is fully zoned in*/
    if (bit_is_pow2_or_zero(index))
    {
        uint32_t cap = (index == 0) ? 1 : index * 2;
        Client** clients = realloc_array_type(zone->clients, cap, Client*);
        
        if (!clients) goto fail;
        
        zone->clients = clients;
    }
    
    zone->clients[index] = client;
    
    client_set_zone(client, zone);
    
    /*fixme: do other initialization here (reset the client for this zone) */
    
    /* Send first set of packets for the client to zone in */
    client_send_player_profile(client);
    client_send_zone_entry(client);
    client_send_weather(client);
    
fail:;
}

Zone* zone_create(LogThread* log, RingBuf* udpQueue, int zoneId, int instId, StaticPackets* staticPackets)
{
    Zone* zone = alloc_type(Zone);
    int rc;
    
    if (!zone) goto fail_alloc;
    
    memset(zone, 0, sizeof(Zone));
    
    zone->zoneId = (int16_t)zoneId;
    zone->instId = (int16_t)instId;
    zone->zoneType = 0xff;
    zone->fog[0].minClippingDistance = 10.0f;
    zone->fog[0].maxClippingDistance = 20000.0f;
    zone->fog[1].minClippingDistance = 10.0f;
    zone->fog[1].maxClippingDistance = 20000.0f;
    zone->fog[2].minClippingDistance = 10.0f;
    zone->fog[2].maxClippingDistance = 20000.0f;
    zone->fog[3].minClippingDistance = 10.0f;
    zone->fog[3].maxClippingDistance = 20000.0f;
    zone->gravity = 0.4f;
    zone->minZ = -32000.0f;
    zone->minClippingDistance = 1000.0f;
    zone->maxClippingDistance = 20000.0f;
    zone->udpQueue = udpQueue;
    zone->logQueue = log_get_queue(log);
    zone->staticPackets = staticPackets;
    
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

const char* zone_short_name(Zone* zone)
{
    return zone_short_name_by_id(zone->zoneId);
}

const char* zone_long_name(Zone* zone)
{
    return zone_long_name_by_id(zone->zoneId);
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

int zone_weather_type(Zone* zone)
{
    return zone->weatherType;
}

int zone_weather_intensity(Zone* zone)
{
    return zone->weatherIntensity;
}

uint16_t zone_sky_type(Zone* zone)
{
    return zone->skyType;
}

uint8_t zone_type(Zone* zone)
{
    return zone->zoneType;
}

ZoneFog* zone_fog(Zone* zone, int n)
{
    return &zone->fog[n];
}

float zone_gravity(Zone* zone)
{
    return zone->gravity;
}

float zone_min_z(Zone* zone)
{
    return zone->minZ;
}

float zone_min_clip_dist(Zone* zone)
{
    return zone->minClippingDistance;
}

float zone_max_clip_dist(Zone* zone)
{
    return zone->maxClippingDistance;
}

LocH* zone_safe_spot(Zone* zone)
{
    return &zone->safeSpot;
}

StaticPackets* zone_static_packets(Zone* zone)
{
    return zone->staticPackets;
}
