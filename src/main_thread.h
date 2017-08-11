
#ifndef MAIN_THREAD_H
#define MAIN_THREAD_H

#include "define.h"
#include "buffer.h"
#include "item_proto.h"
#include "log_thread.h"
#include "ringbuf.h"
#include "udp_thread.h"
#include "util_lua.h"

struct ClientMgr;
struct ZoneMgr;

typedef struct MainThread MainThread;

MainThread* mt_create();
MainThread* mt_destroy(MainThread* mt);

void mt_main_loop(MainThread* mt);
void mt_on_all_zone_threads_shut_down(MainThread* mt);

struct ClientMgr* mt_get_cmgr(MainThread* mt);
struct ZoneMgr* mt_get_zmgr(MainThread* mt);

LogThread* mt_get_log_thread(MainThread* mt);
UdpThread* mt_get_udp_thread(MainThread* mt);

RingBuf* mt_get_queue(MainThread* mt);
RingBuf* mt_get_db_queue(MainThread* mt);
RingBuf* mt_get_log_queue(MainThread* mt);
RingBuf* mt_get_cs_queue(MainThread* mt);

int mt_get_db_id(MainThread* mt);
int mt_get_log_id(MainThread* mt);

StaticBuffer* mt_get_motd(MainThread* mt);

lua_State* mt_get_lua(MainThread* mt);
ItemList* mt_get_item_list(MainThread* mt);

#endif/*MAIN_THREAD_H*/
