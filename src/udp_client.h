
#ifndef UDP_CLIENT_H
#define UDP_CLIENT_H

#include "define.h"
#include "define_netcode.h"
#include "ack_mgr.h"
#include "ringbuf.h"

typedef struct {
    sock_t      sock;
    IpAddr      ipAddr;
    uint32_t    accountId;
    void*       clientObject;
    RingBuf*    toServerQueue;
    RingBuf*    logQueue;
    AckMgr      ackMgr;
} UdpClient;

void udpc_init(UdpClient* udpc, sock_t sock, uint32_t ip, uint16_t port, RingBuf* toServerQueue, RingBuf* logQueue);

bool udpc_recv_protocol(UdpClient* udpc, byte* data, uint32_t len, bool suppressSend);
bool udpc_send_immediate(UdpClient* udpc, const void* data, uint32_t len);

bool udpc_send_pure_ack(UdpClient* udpc, uint16_t ack);

#endif/*UDP_CLIENT_H*/
