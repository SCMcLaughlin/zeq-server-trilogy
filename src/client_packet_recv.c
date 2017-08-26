
#include "client_packet_recv.h"
#include "chat_channel.h"
#include "client_packet_send.h"
#include "log_thread.h"
#include "misc_enum.h"
#include "packet_create.h"
#include "packet_static.h"
#include "packet_struct.h"
#include "util_lua.h"
#include "zone.h"
#include "enum_opcode.h"

static void cpr_warn_unknown_opcode(Client* client, uint16_t opcode)
{
    Zone* zone = client_get_zone(client);
    log_writef(zone_log_queue(zone), zone_log_id(zone), "WARNING: client_packet_recv: received unexpected opcode: 0x%04x", opcode);
}

static void cpr_handle_op_inventory_request(Client* client)
{
    RingBuf* udpQueue = zone_udp_queue(client_get_zone(client));
    
    /*fixme: only respond to this if the client is known to be zoning in*/
    inv_send_all(client_inv(client), client, udpQueue);
}

static void cpr_handle_op_zone_info_request(Client* client)
{
    Zone* zone = client_get_zone(client);
    RingBuf* udpQueue = zone_udp_queue(zone);
    TlgPacket* packet;
    StaticPackets* staticPackets;
    
    /* Send OP_ZoneInfo */
    packet = packet_create_zone_info(zone);
    if (!packet) return;
    client_schedule_packet_with_udp_queue(client, udpQueue, packet);
    
    /* Send OP_EnterZone */
    staticPackets = zone_static_packets(zone);
    client_schedule_packet_with_udp_queue(client, udpQueue, staticPackets->packetEnterZone);
    
    /* Send OP_SpawnsCompressed */
    packet = packet_create_spawns_compressed(zone); /* This may validly return NULL if there are zero spawned Mobs in the zone */
    if (packet)
        client_schedule_packet_with_udp_queue(client, udpQueue, packet);
    
    /*fixme: send doors, objects*/
}

static void cpr_handle_op_enter_zone(Client* client)
{
    Zone* zone = client_get_zone(client);
    RingBuf* udpQueue = zone_udp_queue(zone);
    StaticPackets* staticPackets;
    TlgPacket* packet;
    int rc;
    
    /* Officially add the Client to all the Zone lists, so that we can get them their entityId */
    rc = zone_add_client_fully_zoned_in(zone, client);
    if (rc) goto fail;
    
    /* Inform the client of their entityId */
    packet = packet_create_spawn_appearance(0, SPAWN_APPEARANCE_SetEntityId, client_entity_id(client));
    if (!packet) goto fail;
    client_schedule_packet_with_udp_queue(client, udpQueue, packet);
    
    /* Broadcast the spawn packet, including to the client who is spawning */
    packet = packet_create_spawn(client_mob(client));
    if (!packet) goto fail;
    zone_broadcast_to_all_clients(zone, packet);
    
    /* Send the client some things it expects */
    staticPackets = zone_static_packets(zone);
    client_schedule_packet_with_udp_queue(client, udpQueue, staticPackets->packetEnteredZoneUnknown);
    client_schedule_packet_with_udp_queue(client, udpQueue, staticPackets->packetEnterZone);
    
    /*fixme: Send guild rank SpawnAppearance, if applicable */
    /*fixme: Send GM SpawnAppearance, if applicable */
    
    /* Send HP and Mana updates */
    client_update_hp_with_current(client);
    client_update_mana_with_current(client);
    
    /* Execute spawn events */
    zlua_add_client_to_zone_lists(client, zone);
    
    if (client_increment_zone_in_count(client) == 0)
        zlua_event_literal("event_connect", client_mob(client), zone, NULL);
    
    zlua_event_literal("event_spawn", client_mob(client), zone, NULL);
    
fail:;
}

static void cpr_handle_op_message(Client* client, ToServerPacket* packet)
{
    const char* targetName;
    const char* senderName;
    const char* msg;
    int targetLen;
    int senderLen;
    int msgLen;
    uint16_t langId;
    uint16_t chatChannel;
    uint32_t len;
    Aligned a;
    
    len = packet->length;
    
    if (len < (sizeof(PS_Message) + 2))
        return;
    
    aligned_init(&a, packet->data, len);
    
    /* PS_Message */
    /* targetName */
    targetName = aligned_read_string_bounded(&a, &targetLen, sizeof_field(PS_Message, targetName));
    /* senderName */
    senderName = aligned_read_string_bounded(&a, &senderLen, sizeof_field(PS_Message, senderName));
    /* languageId */
    langId = aligned_read_uint16(&a);
    /* chatChannel */
    chatChannel = aligned_read_uint16(&a);
    /* unknown */
    aligned_advance_by_sizeof(&a, uint16_t);
    /* message */
    msg = aligned_read_string_bounded(&a, &msgLen, len - sizeof(PS_Message));
    
    switch (chatChannel)
    {
    case CHAT_CHANNEL_Say:
        if (msg[0] == '#')
        {
            client_on_msg_command(client, msg, msgLen);
            break;
        }
        break;
    
    default:
        break;
    }
}

static void cpr_handle_op_target(Client* client, ToServerPacket* packet)
{
    Aligned a;
    uint32_t len = packet->length;
    uint32_t entityId;

    if (len != sizeof(PS_Target))
        return;
    
    aligned_init(&a, packet->data, len);

    /* PS_Target */
    /* entityId */
    entityId = aligned_read_uint32(&a);

    client_on_target_by_entity_id(client, (int16_t)entityId);
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
        cpr_handle_op_inventory_request(client);
        break;
    
    case OP_ZoneInfoRequest:
        cpr_handle_op_zone_info_request(client);
        break;
    
    case OP_EnterZone:
        cpr_handle_op_enter_zone(client);
        break;
    
    case OP_Message:
        cpr_handle_op_message(client, &packet);
        break;

    case OP_Target:
        cpr_handle_op_target(client, &packet);
        break;
    
    /* Packets to be echoed with all their content */
    case OP_ZoneInUnknown:
        client_send_echo_copy(client, &packet);
        break;
    
    default:
        client_on_unhandled_packet(client, &packet);
        cpr_warn_unknown_opcode(client, packet.opcode);
        break;
    }
    
    if (packet.data)
        free(packet.data);
}
