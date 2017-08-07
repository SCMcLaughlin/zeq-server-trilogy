
#ifndef LOGIN_THREAD_H
#define LOGIN_THREAD_H

#include "define.h"
#include "buffer.h"
#include "ringbuf.h"
#include "log_thread.h"
#include "udp_thread.h"

typedef struct LoginThread LoginThread;

LoginThread* login_create(RingBuf* mainQueue, LogThread* log, RingBuf* dbQueue, int dbId, UdpThread* udp, RingBuf* csQueue);
LoginThread* login_destroy(LoginThread* login);

RingBuf* login_get_queue(LoginThread* login);

int login_add_server(LoginThread* login, int* outServerId, const char* name, StaticBuffer* remoteIp, StaticBuffer* localIp, int8_t rank, int8_t status, bool isLocal);

#endif/*LOGIN_THREAD_H*/
