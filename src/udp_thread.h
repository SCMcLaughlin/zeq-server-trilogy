
#ifndef UDP_THREAD_H
#define UDP_THREAD_H

#include "define.h"
#include "ringbuf.h"
#include "log_thread.h"

typedef struct UdpThread UdpThread;

UdpThread* udp_create(LogThread* log);
UdpThread* udp_destroy(UdpThread* udp);
int udp_trigger_shutdown(UdpThread* udp);
int udp_open_port(UdpThread* udp, uint16_t port, uint32_t clientSize, RingBuf* toServerQueue, RingBuf* toClientQueue);

#endif/*UDP_THREAD_H*/
