
#include "main_timers.h"
#include "main_thread.h"
#include "ringbuf.h"
#include "zpacket.h"
#include "enum_zop.h"

void mtt_cs_auth_timeouts(TimerPool* pool, Timer* timer)
{
    MainThread* mt = timer_userdata_type(timer, MainThread);
    RingBuf* csQueue = mt_get_cs_queue(mt);

    (void)pool;

    ringbuf_push(csQueue, ZOP_CS_CheckAuthTimeouts, NULL);
}
