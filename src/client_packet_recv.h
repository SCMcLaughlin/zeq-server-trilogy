
#ifndef CLIENT_PACKET_RECV_H
#define CLIENT_PACKET_RECV_H

#include "define.h"
#include "client.h"
#include "zpacket.h"

void client_packet_recv(Client* client, ZPacket* zpacket);

#endif/*CLIENT_PACKET_RECV_H*/
