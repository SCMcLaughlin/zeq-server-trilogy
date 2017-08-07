
#ifndef PACKET_STATIC_H
#define PACKET_STATIC_H

#include "define.h"
#include "tlg_packet.h"

typedef struct {
    TlgPacket*  packetEnterZone;
    TlgPacket*  packetEnteredZoneUnknown;
} StaticPackets;

int static_packets_init(StaticPackets* sp);
void static_packets_deinit(StaticPackets* sp);

#endif/*PACKET_STATIC_H*/
