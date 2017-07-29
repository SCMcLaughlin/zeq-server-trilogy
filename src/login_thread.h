
#ifndef LOGIN_THREAD_H
#define LOGIN_THREAD_H

#include "define.h"
#include "log_thread.h"

typedef struct LoginThread LoginThread;

LoginThread* login_create(LogThread* log, RingBuf* dbQueue, int dbId);
LoginThread* login_destroy(LoginThread* login);

#endif/*LOGIN_THREAD_H*/
