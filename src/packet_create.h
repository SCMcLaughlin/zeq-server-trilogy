
#ifndef PACKET_CREATE_H
#define PACKET_CREATE_H

#include "define.h"
#include "tlg_packet.h"
#include "zone.h"

TlgPacket* packet_create_zero_filled(uint16_t opcode, uint32_t size);
TlgPacket* packet_create_weather(int type, int intensity);
TlgPacket* packet_create_zone_info(Zone* zone);
TlgPacket* packet_create_spawn_appearance(int16_t entityId, int16_t typeId, int value);

#endif/*PACKET_CREATE_H*/
