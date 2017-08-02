
#ifndef UDP_THREAD_H
#define UDP_THREAD_H

#include "define.h"
#include "define_netcode.h"
#include "ringbuf.h"
#include "tlg_packet.h"
#include "log_thread.h"

typedef struct UdpThread UdpThread;

UdpThread* udp_create(LogThread* log);
UdpThread* udp_destroy(UdpThread* udp);
int udp_trigger_shutdown(UdpThread* udp);
int udp_open_port(UdpThread* udp, uint16_t port, uint32_t clientSize, RingBuf* toServerQueue);

RingBuf* udp_get_queue(UdpThread* udp);

int udp_schedule_packet(RingBuf* toClientQueue, IpAddr ipAddr, TlgPacket* packet);

RingBuf* udp_get_log_queue(UdpThread* udp);
int udp_get_log_id(UdpThread* udp);

/* Only to be used by UdpClient */
void udp_thread_update_ack_timestamp(UdpThread* udp, uint32_t index);

#endif/*UDP_THREAD_H*/
