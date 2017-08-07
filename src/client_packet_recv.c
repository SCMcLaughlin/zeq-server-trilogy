
#include "client_packet_recv.h"
#include "client_packet_send.h"
#include "log_thread.h"
#include "packet_create.h"
#include "packet_static.h"
#include "zone.h"
#include "enum_opcode.h"

static void cpr_warn_unknown_opcode(Client* client, uint16_t opcode)
{
    Zone* zone = client_get_zone(client);
    log_writef(zone_log_queue(zone), zone_log_id(zone), "WARNING: client_packet_recv: received unexpected opcode: 0x%04x", opcode);
}

static void cpr_handle_op_zone_info_request(Client* client)
{
    Zone* zone = client_get_zone(client);
    TlgPacket* packet;
    StaticPackets* staticPackets;
    
    /* Send OP_ZoneInfo */
    packet = packet_create_zone_info(zone);
    if (!packet) return;
    client_schedule_packet(client, packet);
    
    /* Send OP_EnterZone */
    staticPackets = zone_static_packets(zone);
    client_schedule_packet(client, staticPackets->packetEnterZone);
    
    /*fixme: send spawns, doors, objects*/
}

void client_packet_recv(Client* client, ZPacket* zpacket)
{
    ToServerPacket packet;
    
    packet.opcode = zpacket->udp.zToServerPacket.opcode;
    packet.length = zpacket->udp.zToServerPacket.length;
    packet.data = zpacket->udp.zToServerPacket.data;
    
    switch (packet.opcode)
    {
    case OP_InventoryRequest:
        break;
    
    case OP_ZoneInfoRequest:
        cpr_handle_op_zone_info_request(client);
        break;
    
    case OP_EnterZone:
        break;
    
    /* Packets to be echoed with all their content */
    case OP_ZoneInUnknown:
        client_send_echo_copy(client, &packet);
        break;
    
    default:
        cpr_warn_unknown_opcode(client, packet.opcode);
        break;
    }
    
    if (packet.data)
        free(packet.data);
}
