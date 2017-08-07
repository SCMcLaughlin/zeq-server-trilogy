
#ifndef CLIENT_PACKET_SEND_H
#define CLIENT_PACKET_SEND_H

#include "define.h"
#include "client.h"
#include "ringbuf.h"
#include "tlg_packet.h"
#include "udp_zpacket.h"
#include "zone.h"

void client_schedule_packet(Client* client, TlgPacket* packet);
void client_schedule_packet_with_zone(Client* client, Zone* zone, TlgPacket* packet);
void client_schedule_packet_with_udp_queue(Client* client, RingBuf* udpQueue, TlgPacket* packet);
void client_send_echo_copy(Client* client, ToServerPacket* packet);
void client_send_player_profile(Client* client);
void client_send_zone_entry(Client* client);
void client_send_weather(Client* client);

#endif/*CLIENT_PACKET_SEND_H*/
