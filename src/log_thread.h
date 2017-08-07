
#ifndef LOG_THREAD_H
#define LOG_THREAD_H

#include "define.h"
#include "buffer.h"
#include "ringbuf.h"

typedef struct LogThread LogThread;

LogThread* log_create(RingBuf* mainQueue);
LogThread* log_destroy(LogThread* log);

RingBuf* log_get_queue(LogThread* log);

int log_write(RingBuf* logQueue, int logId, const char* msg, int len);
#define log_write_literal(rb, logId, str) log_write((rb), (logId), (str), sizeof(str) - 1)
int log_write_sbuf(RingBuf* logQueue, int logId, StaticBuffer* sbuf);
int log_writef(RingBuf* logQueue, int logId, const char* fmt, ...);
int log_open_file(LogThread* log, int* outLogId, const char* path, int len);
#define log_open_file_literal(log, outId, path) log_open_file((log), (outId), path, sizeof(path) - 1)
int log_open_filef(LogThread* log, int* outLogId, const char* fmt, ...);
int log_close_file(RingBuf* logQueue, int logId);

#endif/*LOG_THREAD_H*/
