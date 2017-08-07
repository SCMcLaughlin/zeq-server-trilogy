
#include "packet_static.h"
#include "packet_create.h"
#include "enum_opcode.h"

int static_packets_init(StaticPackets* sp)
{
    sp->packetEnterZone = packet_create(OP_EnterZone, 0);
    if (!sp->packetEnterZone) goto oom;
    packet_grab(sp->packetEnterZone);
    
    sp->packetEnteredZoneUnknown = packet_create_zero_filled(OP_EnteredZoneUnknown, 8);
    if (!sp->packetEnteredZoneUnknown) goto oom;
    packet_grab(sp->packetEnteredZoneUnknown);
        
    return ERR_None;
oom:
    return ERR_OutOfMemory;
}

void static_packets_deinit(StaticPackets* sp)
{
    sp->packetEnterZone = packet_drop(sp->packetEnterZone);
    sp->packetEnteredZoneUnknown = packet_drop(sp->packetEnteredZoneUnknown);
}
