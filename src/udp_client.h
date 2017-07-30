
#ifndef UDP_CLIENT_H
#define UDP_CLIENT_H

#include "define.h"
#include "define_netcode.h"
#include "ack_mgr.h"
#include "ringbuf.h"
#include "tlg_packet.h"

typedef struct UdpClient {
    sock_t      sock;
    IpAddr      ipAddr;
    uint32_t    accountId;
    int         logId;
    void*       clientObject;
    RingBuf*    toServerQueue;
    RingBuf*    logQueue;
    AckMgr      ackMgr;
} UdpClient;

void udpc_init(UdpClient* udpc, sock_t sock, uint32_t ip, uint16_t port, RingBuf* toServerQueue, RingBuf* logQueue, int logId);

bool udpc_send_disconnect(UdpClient* udpc);
void udpc_recv_packet(UdpClient* udpc, Aligned* a, uint16_t opcode);
void udpc_recv_packet_no_copy(UdpClient* udpc, Aligned* a, uint16_t opcode);
bool udpc_recv_protocol(UdpClient* udpc, byte* data, uint32_t len, bool suppressSend);
void udpc_schedule_packet(UdpClient* udpc, TlgPacket* packet, bool noAckRequest);
bool udpc_send_immediate(UdpClient* udpc, const void* data, uint32_t len);
void udpc_send_queued_packets(UdpClient* udpc);

bool udpc_send_pure_ack(UdpClient* udpc, uint16_t ackNetworkByteOrder);

#endif/*UDP_CLIENT_H*/
