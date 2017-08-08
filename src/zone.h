
#ifndef ZONE_H
#define ZONE_H

#include "define.h"
#include "loc.h"
#include "log_thread.h"
#include "misc_struct.h"
#include "packet_static.h"
#include "tlg_packet.h"

typedef struct Zone Zone;

struct Client;

Zone* zone_create(LogThread* log, RingBuf* udpQueue, int zoneId, int instId, StaticPackets* staticPackets);
Zone* zone_destroy(Zone* zone);

void zone_add_client_zoning_in(Zone* zone, struct Client* client);
int zone_add_client_fully_zoned_in(Zone* zone, struct Client* client);

void zone_broadcast_to_all_clients(Zone* zone, TlgPacket* packet);

const char* zone_short_name(Zone* zone);
const char* zone_long_name(Zone* zone);
int16_t zone_id(Zone* zone);
int16_t zone_inst_id(Zone* zone);

RingBuf* zone_udp_queue(Zone* zone);
RingBuf* zone_log_queue(Zone* zone);
int zone_log_id(Zone* zone);

int zone_weather_type(Zone* zone);
int zone_weather_intensity(Zone* zone);

uint16_t zone_sky_type(Zone* zone);
uint8_t zone_type(Zone* zone);
ZoneFog* zone_fog(Zone* zone, int n);
float zone_gravity(Zone* zone);
float zone_min_z(Zone* zone);
float zone_min_clip_dist(Zone* zone);
float zone_max_clip_dist(Zone* zone);
LocH* zone_safe_spot(Zone* zone);

StaticPackets* zone_static_packets(Zone* zone);

#endif/*ZONE_H*/
