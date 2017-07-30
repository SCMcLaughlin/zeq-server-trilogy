
#include "main_thread.h"
#include "db_thread.h"
#include "log_thread.h"
#include "login_thread.h"
#include "login_client.h"
#include "udp_thread.h"
#include "ringbuf.h"
#include "util_alloc.h"
#include "util_clock.h"
#include "zpacket.h"
#include "enum2str.h"

#define MAIN_THREAD_LOG_PATH "log/main_thread.txt"

struct MainThread {
    DbThread*       db;
    LogThread*      log;
    UdpThread*      udp;
    LoginThread*    login;
    RingBuf*        mainQueue;
    RingBuf*        logQueue;
    RingBuf*        dbQueue;
    int             dbId;
    int             logId;
};

MainThread* mt_create()
{
    MainThread* mt = alloc_type(MainThread);
    int rc;

    if (!mt)
    {
        fprintf(stderr, "mt_create: failed to allocate MainThread object\n");
        goto fail_alloc;
    }

    memset(mt, 0, sizeof(MainThread));
    mt->mainQueue = ringbuf_create_type(ZPacket, 1024);
    if (!mt->mainQueue)
    {
        fprintf(stderr, "mt_create: failed to create RingBuf object for MainThread\n");
        goto fail;
    }

    /* Start the LogThread before anything else */
    mt->log = log_create();
    if (!mt->log)
    {
        fprintf(stderr, "mt_create: failed to create LogThread object\n");
        goto fail;
    }

    mt->logQueue = log_get_queue(mt->log);
    rc = log_open_file_literal(mt->log, &mt->logId, MAIN_THREAD_LOG_PATH);
    if (rc)
    {
        fprintf(stderr, "mt_create: failed to open log file for MainThread at '" MAIN_THREAD_LOG_PATH "': %s\n", enum2str_err(rc));
        goto fail;
    }
    
    mt->db = db_create(mt->log);
    if (!mt->db)
    {
        fprintf(stderr, "mt_create: failed to create DbThread object\n");
        goto fail;
    }
    
    mt->dbQueue = db_get_queue(mt->db);
    rc = db_open_literal(mt->db, mt->mainQueue, &mt->dbId, DB_THREAD_DB_PATH_DEFAULT, DB_THREAD_DB_SCHEMA_PATH_DEFAULT);
    if (rc)
    {
        fprintf(stderr, "mt_create: failed to open database file '" DB_THREAD_DB_PATH_DEFAULT "', schema '" DB_THREAD_DB_SCHEMA_PATH_DEFAULT "': %s\n", enum2str_err(rc));
        goto fail;
    }

    mt->udp = udp_create(mt->log);
    if (!mt->udp)
    {
        fprintf(stderr, "mt_create: failed to create UdpThread object\n");
        goto fail;
    }

    return mt;

fail:
    mt_destroy(mt);
fail_alloc:
    return NULL;
}

MainThread* mt_destroy(MainThread* mt)
{
    if (mt)
    {
        if (mt->udp)
        {
            udp_trigger_shutdown(mt->udp);
            //fixme: wait here, then destroy udp
        }
        
        if (mt->db)
        {
            //fixme: shut down
            mt->db = db_destroy(mt->db);
        }

        if (mt->log)
        {
            log_shut_down(mt->log);
            mt->log = log_destroy(mt->log);
        }

        mt->mainQueue = ringbuf_destroy(mt->mainQueue);
        free(mt);
    }

    return NULL;
}

void mt_main_loop(MainThread* mt)
{
    int rc;
    
    mt->login = login_create(mt->log, mt->dbQueue, mt->dbId, udp_get_queue(mt->udp));
    if (!mt->login)
    {
        fprintf(stderr, "login_create() failed\n");
        return;
    }
    
    rc = udp_open_port(mt->udp, 5998, loginc_sizeof(), login_get_queue(mt->login));
    if (rc)
    {
        fprintf(stderr, "udp_open_port() failed: %s\n", enum2str_err(rc));
        return;
    }
    
    for (;;)
    {
        clock_sleep(50);
    }
}
