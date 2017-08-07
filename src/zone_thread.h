
#ifndef ZONE_THREAD_H
#define ZONE_THREAD_H

#include "define.h"
#include "ringbuf.h"
#include "log_thread.h"
#include "packet_static.h"
#include "udp_thread.h"

typedef struct ZoneThread ZoneThread;

ZoneThread* zt_create(RingBuf* mainQueue, LogThread* log, RingBuf* dbQueue, int dbId, UdpThread* udp, uint32_t index, uint16_t port, StaticPackets* staticPackets);
ZoneThread* zt_destroy(ZoneThread* zt);

RingBuf* zt_get_queue(ZoneThread* zt);

#endif/*ZONE_THREAD_H*/
