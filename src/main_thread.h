
#ifndef MAIN_THREAD_H
#define MAIN_THREAD_H

#include "define.h"
#include "ringbuf.h"

struct ClientMgr;

typedef struct MainThread MainThread;

MainThread* mt_create();
MainThread* mt_destroy(MainThread* mt);

void mt_main_loop(MainThread* mt);

struct ClientMgr* mt_get_cmgr(MainThread* mt);

RingBuf* mt_get_queue(MainThread* mt);
RingBuf* mt_get_db_queue(MainThread* mt);
RingBuf* mt_get_log_queue(MainThread* mt);
RingBuf* mt_get_cs_queue(MainThread* mt);

int mt_get_db_id(MainThread* mt);
int mt_get_log_id(MainThread* mt);

#endif/*MAIN_THREAD_H*/
