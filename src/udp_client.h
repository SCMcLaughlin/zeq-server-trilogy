
#ifndef UDP_CLIENT_H
#define UDP_CLIENT_H

#include "define.h"
#include "define_netcode.h"
#include "ack_mgr.h"
#include "ringbuf.h"
#include "tlg_packet.h"

struct UdpThread;

typedef struct UdpClient {
    sock_t      sock;
    IpAddr      ipAddr;
    uint32_t    accountId;
    int         udpIndex;
    void*       clientObject;
    RingBuf*    toServerQueue;
    AckMgr      ackMgr;
} UdpClient;

void udpc_init(UdpClient* udpc, sock_t sock, uint32_t ip, uint16_t port, RingBuf* toServerQueue);
void udpc_deinit(UdpClient* udpc);

#define udpc_set_index(udpc, idx) ((udpc)->udpIndex = (int)(idx))

void udpc_linkdead(struct UdpThread* udp, UdpClient* udpc);
bool udpc_send_disconnect(struct UdpThread* udp, UdpClient* udpc);
void udpc_recv_packet(struct UdpThread* udp, UdpClient* udpc, Aligned* a, uint16_t opcode);
void udpc_recv_packet_no_copy(struct UdpThread* udp, UdpClient* udpc, Aligned* a, uint16_t opcode);
bool udpc_recv_protocol(struct UdpThread* udp, UdpClient* udpc, byte* data, uint32_t len, bool suppressRecv);
void udpc_schedule_packet(UdpClient* udpc, TlgPacket* packet, bool noAckRequest);
bool udpc_send_immediate(UdpClient* udpc, const void* data, uint32_t len);
void udpc_send_queued_packets(struct UdpThread* udp, UdpClient* udpc);

void udpc_flag_last_ack(struct UdpThread* udp, UdpClient* udpc);
bool udpc_send_pure_ack(struct UdpThread* udp, UdpClient* udpc, uint16_t ackNetworkByteOrder);
bool udpc_send_keep_alive_ack(UdpClient* udpc);

#endif/*UDP_CLIENT_H*/
