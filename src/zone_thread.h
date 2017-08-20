
#ifndef ZONE_THREAD_H
#define ZONE_THREAD_H

#include "define.h"
#include "ringbuf.h"
#include "log_thread.h"
#include "packet_static.h"
#include "timer.h"
#include "udp_thread.h"
#include "util_lua.h"

typedef struct ZoneThread ZoneThread;

ZoneThread* zt_create(RingBuf* mainQueue, LogThread* log, RingBuf* dbQueue, int dbId, UdpThread* udp, uint32_t index, uint16_t port, StaticPackets* staticPackets);
ZoneThread* zt_destroy(ZoneThread* zt);

RingBuf* zt_get_queue(ZoneThread* zt);
ZEQ_API RingBuf* zt_get_log_queue(ZoneThread* zt);
ZEQ_API int zt_get_log_id(ZoneThread* zt);

ZEQ_API TimerPool* zt_timer_pool(ZoneThread* zt);
ZEQ_API lua_State* zt_lua(ZoneThread* zt);

#endif/*ZONE_THREAD_H*/
