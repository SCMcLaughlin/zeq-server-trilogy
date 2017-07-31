
#ifndef CHAR_SELECT_THREAD_H
#define CHAR_SELECT_THREAD_H

#include "define.h"
#include "ringbuf.h"
#include "log_thread.h"
#include "udp_thread.h"

typedef struct CharSelectThread CharSelectThread;

CharSelectThread* cs_create(LogThread* log, RingBuf* dbQueue, int dbId, UdpThread* udp);
CharSelectThread* cs_destroy(CharSelectThread* cs);

RingBuf* cs_get_queue(CharSelectThread* cs);

#endif/*CHAR_SELECT_THREAD_H*/
