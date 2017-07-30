
#ifndef LOGIN_THREAD_H
#define LOGIN_THREAD_H

#include "define.h"
#include "log_thread.h"

typedef struct LoginThread LoginThread;

LoginThread* login_create(LogThread* log, RingBuf* dbQueue, int dbId);
LoginThread* login_destroy(LoginThread* login);

int login_add_server(LoginThread* login, int* outServerId, const char* name, const char* remoteIp, const char* localIp, int8_t rank, int8_t status, bool isLocal);

#endif/*LOGIN_THREAD_H*/
