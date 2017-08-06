
#ifndef CLIENT_PACKET_SEND_H
#define CLIENT_PACKET_SEND_H

#include "define.h"
#include "client.h"
#include "tlg_packet.h"
#include "udp_zpacket.h"

void client_schedule_packet(Client* client, TlgPacket* packet);
void client_send_echo_copy(Client* client, ToServerPacket* packet);
void client_send_player_profile(Client* client);
void client_send_zone_entry(Client* client);

#endif/*CLIENT_PACKET_SEND_H*/
