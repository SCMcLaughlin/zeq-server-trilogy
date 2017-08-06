
#include "client_packet_recv.h"
#include "log_thread.h"
#include "zone.h"

static void cpr_warn_unknown_opcode(Client* client, uint16_t opcode)
{
    Zone* zone = client_get_zone(client);
    log_writef(zone_log_queue(zone), zone_log_id(zone), "WARNING: client_packet_recv: received unexpected opcode: 0x%04x", opcode);
}

void client_packet_recv(Client* client, ZPacket* zpacket)
{
    ToServerPacket packet;
    
    packet.opcode = zpacket->udp.zToServerPacket.opcode;
    packet.length = zpacket->udp.zToServerPacket.length;
    packet.data = zpacket->udp.zToServerPacket.data;
    
    switch (packet.opcode)
    {
    default:
        cpr_warn_unknown_opcode(client, packet.opcode);
        break;
    }
    
    if (packet.data)
        free(packet.data);
}
