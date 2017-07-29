
#include "log_thread.h"
#include "bit.h"
#include "util_alloc.h"
#include "util_atomic.h"
#include "util_clock.h"
#include "util_file.h"
#include "util_thread.h"
#include "enum_zop.h"
#include <time.h>

#define LOG_THREAD_MSG_QUEUE_SIZE 1024
#define LOG_THREAD_SHUTDOWN_TIMEOUT_MS 5000

/*fixme: todo:
    * Add log file continuation (open existing) with messages saying when the log was opened or closed
    * Add log file archiving and compression past a specific byte size (using libarchive)
*/

typedef struct {
    int         logId;
    uint32_t    bytes;
    FILE*       fp;
} LogFile;

typedef struct {
    int             logId;
    StaticBuffer*   sbuf;
} LogMsg;

struct LogThread {
    RingBuf*    logQueue;
    LogFile*    logFiles;
    uint32_t    logFileCount;
    atomic32_t  nextLogId;
    atomic8_t   isThreadShutDown;
};

static uint32_t log_thread_write_timestamp(FILE* fp)
{
    char buf[256];
    time_t rawTime;
    struct tm* curTime;
    int len;
    
    rawTime = time(NULL);
    
    if (rawTime == -1) goto zero;
    
    curTime = localtime(&rawTime);
    
    if (!curTime) goto zero;
    
    len = strftime(buf, sizeof(buf), "[%Y-%m-%d|%H:%M:%S] ", curTime);
    
    if (len <= 0) goto zero;
    
    return fwrite(buf, sizeof(char), len, fp);
    
zero:
    return 0;
}

static void log_thread_write(LogThread* log, LogMsg* packet)
{
    int logId = packet->logId;
    StaticBuffer* sbuf = packet->sbuf;
    LogFile* files = log->logFiles;
    uint32_t n = log->logFileCount;
    uint32_t i;

    for (i = 0; i < n; i++)
    {
        LogFile* file = &files[i];
        size_t written = 1;
        FILE* fp;

        if (file->logId != logId)
            continue;

        fp = file->fp;

        written += log_thread_write_timestamp(fp);

        if (sbuf)
        {
            written += fwrite(sbuf_data(sbuf), sizeof(char), sbuf_length(sbuf), fp);
        }

        fputc('\n', fp);
        file->bytes += written;
        break;
    }

    if (sbuf) sbuf_drop(sbuf);
}

static void log_thread_open_file(LogThread* log, LogMsg* packet)
{
    int logId = packet->logId;
    StaticBuffer* sbuf = packet->sbuf;
    FILE* fp = fopen(sbuf_str(sbuf), "ab");
    LogFile* cur;
    uint32_t index;
    uint32_t written;

    if (!fp) goto failure;

    index = log->logFileCount;

    if (bit_is_pow2_or_zero(index))
    {
        uint32_t cap = (index == 0) ? 1 : (index * 2);
        LogFile* array = realloc_array_type(log->logFiles, cap, LogFile);

        if (!array) goto failure;

        log->logFiles = array;
    }

    log->logFileCount = index + 1;
    cur = &log->logFiles[index];
    cur->logId = logId;
    cur->fp = fp;

    fseek(fp, 0, SEEK_END);
    written = ftell(fp);
    
    written += log_thread_write_timestamp(fp);
    written += fwrite_literal(fp, "::: Opened Log :::\n");
    
    cur->bytes = written;

failure:
    if (sbuf) sbuf_drop(sbuf);
}

static void log_thread_close_file(LogThread* log, LogMsg* packet)
{
    int logId = packet->logId;
    LogFile* files = log->logFiles;
    uint32_t n = log->logFileCount;
    uint32_t i;

    if (files)
    {
        for (i = 0; i < n; i++)
        {
            LogFile* file = &files[i];

            if (file->logId != logId)
                continue;

            if (file->fp)
            {
                fclose(file->fp);
                file->fp = NULL;
            }

            /* Pop and swap */
            n--;
            files[i] = files[n];
            log->logFileCount = n;
            break;
        }
    }
}

static void log_thread_proc(void* ptr)
{
    LogThread* log = (LogThread*)ptr;
    RingBuf* logQueue = log->logQueue;
    LogMsg packet;
    int zop;
    int rc;

    for (;;)
    {
        rc = ringbuf_wait_for_packet(logQueue, &zop, &packet);
        if (rc) goto terminate;

        switch (zop)
        {
        case ZOP_LOG_TerminateThread:
            goto terminate;

        case ZOP_LOG_Write:
            log_thread_write(log, &packet);
            break;

        case ZOP_LOG_OpenFile:
            log_thread_open_file(log, &packet);
            break;

        case ZOP_LOG_CloseFile:
            log_thread_close_file(log, &packet);
            break;

        default:
            break;
        }
    }

terminate:
    atomic8_set(&log->isThreadShutDown, 1);
}

LogThread* log_create()
{
    LogThread* log = alloc_type(LogThread);
    int rc;

    if (!log) goto fail_alloc;

    log->logQueue = NULL;
    log->logFiles = NULL;
    log->logFileCount = 0;
    atomic32_set(&log->nextLogId, 1);
    atomic8_set(&log->isThreadShutDown, 0);

    log->logQueue = ringbuf_create_type(LogMsg, LOG_THREAD_MSG_QUEUE_SIZE);
    if (!log->logQueue) goto fail;

    rc = thread_start(log_thread_proc, log);
    if (rc) goto fail;

    return log;

fail:
    log_destroy(log);
fail_alloc:
    return NULL;
}

static void log_close_all_files(LogThread* log)
{
    LogFile* files = log->logFiles;
    uint32_t count = log->logFileCount;
    uint32_t i;

    if (files)
    {
        for (i = 0; i < count; i++)
        {
            LogFile* file = &files[i];

            if (file->fp)
            {
                fclose(file->fp);
                file->fp = NULL;
            }
        }

        free(files);
        log->logFiles = NULL;
        log->logFileCount = 0;
    }
}

LogThread* log_destroy(LogThread* log)
{
    if (log)
    {
        log_close_all_files(log);
        ringbuf_destroy_if_exists(log->logQueue);
        free(log);
    }

    return NULL;
}

void log_shut_down(LogThread* log)
{
    if (log)
    {
        int rc = ringbuf_push(log->logQueue, ZOP_LOG_TerminateThread, NULL);

        if (rc == ERR_None)
        {
            uint64_t timeout = clock_milliseconds() + LOG_THREAD_SHUTDOWN_TIMEOUT_MS;

            for (;;)
            {
                if (atomic8_get(&log->isThreadShutDown) != 0)
                    break;

                if (clock_milliseconds() >= timeout)
                    break;
            }
        }
    }
}

RingBuf* log_get_queue(LogThread* log)
{
    return log->logQueue;
}

static int log_write_op_sbuf(RingBuf* logQueue, int zop, int logId, StaticBuffer* sbuf)
{
    LogMsg packet;
    int rc;

    packet.logId = logId;
    packet.sbuf = sbuf;

    if (sbuf) sbuf_grab(sbuf);

    rc = ringbuf_push(logQueue, zop, &packet);
    if (rc && sbuf) sbuf_drop(sbuf);

    return rc;
}

static int log_write_op(RingBuf* logQueue, int zop, int logId, const char* msg, int len)
{
    StaticBuffer* sbuf = NULL;

    if (msg)
    {
        if (len < 0)
            len = strlen(msg);

        sbuf = sbuf_create(msg, len);
        if (!sbuf) return ERR_OutOfMemory;
    }

    return log_write_op_sbuf(logQueue, zop, logId, sbuf);
}

int log_write(RingBuf* logQueue, int logId, const char* msg, int len)
{
    return log_write_op(logQueue, ZOP_LOG_Write, logId, msg, len);
}

int log_write_sbuf(RingBuf* logQueue, int logId, StaticBuffer* sbuf)
{
    return log_write_op_sbuf(logQueue, ZOP_LOG_Write, logId, sbuf);
}

int log_writef(RingBuf* logQueue, int logId, const char* fmt, ...)
{
    char buf[4096];
    int len;
    va_list args;

    va_start(args, fmt);
    len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    if (len <= 0 || len >= (int)sizeof(buf))
        return ERR_Generic;

    return log_write(logQueue, logId, buf, len);
}

int log_open_file(LogThread* log, int* outLogId, const char* path, int len)
{
    int logId = atomic32_add(&log->nextLogId, 1);

    if (outLogId)
        *outLogId = logId;

    return log_write_op(log->logQueue, ZOP_LOG_OpenFile, logId, path, len);
}

int log_close_file(RingBuf* logQueue, int logId)
{
    return log_write_op_sbuf(logQueue, ZOP_LOG_CloseFile, logId, NULL);
}
