
#ifndef ZONE_H
#define ZONE_H

#include "define.h"
#include "loc.h"
#include "log_thread.h"
#include "misc_struct.h"
#include "packet_static.h"
#include "tlg_packet.h"
#include "util_lua.h"

typedef struct Zone Zone;

struct Client;
struct Mob;

Zone* zone_create(LogThread* log, RingBuf* udpQueue, int zoneId, int instId, StaticPackets* staticPackets, lua_State* L);
Zone* zone_destroy(Zone* zone);

void zone_add_client_zoning_in(Zone* zone, struct Client* client);
int zone_add_client_fully_zoned_in(Zone* zone, struct Client* client);

#define zone_broadcast_to_all_clients(zone, packet) zone_broadcast_to_all_clients_except((zone), (packet), NULL)
void zone_broadcast_to_all_clients_except(Zone* zone, TlgPacket* packet, struct Client* except);
void zone_broadcast_to_nearby_clients_except(Zone* zone, TlgPacket* packet, double x, double y, double z, double range, struct Mob* except);

ZEQ_API const char* zone_short_name(Zone* zone);
ZEQ_API const char* zone_long_name(Zone* zone);
int16_t zone_id(Zone* zone);
int16_t zone_inst_id(Zone* zone);

RingBuf* zone_udp_queue(Zone* zone);
ZEQ_API RingBuf* zone_log_queue(Zone* zone);
ZEQ_API int zone_log_id(Zone* zone);

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
void zone_set_lua_index(Zone* zone, int index);
int zone_lua_index(Zone* zone);
lua_State* zone_lua(Zone* zone);

struct Mob** zone_mob_list(Zone* zone);
uint32_t zone_mob_count(Zone* zone);

#endif/*ZONE_H*/
