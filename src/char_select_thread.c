
#include "char_select_thread.h"
#include "util_alloc.h"
#include "util_thread.h"
#include "zpacket.h"
#include "char_select_packet_struct.h"
#include "enum_char_select_opcode.h"
#include "enum_zop.h"
#include "enum2str.h"

#define CHAR_SELECT_THREAD_UDP_PORT 9000
#define CHAR_SELECT_THREAD_LOG_PATH "log/char_select_thread.txt"

struct CharSelectThread {
    RingBuf*    csQueue;
    RingBuf*    udpQueue;
    RingBuf*    dbQueue;
    RingBuf*    logQueue;
    int         dbId;
    int         logId;
};

static void cs_thread_proc(void* ptr)
{
    CharSelectThread* cs = (CharSelectThread*)ptr;
    RingBuf* csQueue = cs->csQueue;
    ZPacket zpacket;
    int zop;
    int rc;
    
    for (;;)
    {
        /* The char select thread is purely packet-driven, always blocks until something needs to be done */
        rc = ringbuf_wait_for_packet(csQueue, &zop, &zpacket);
        
        if (rc)
        {
            log_writef(cs->logQueue, cs->logId, "cs_thread_proc: ringbuf_wait_for_packet() returned error: %s", enum2str_err(rc));
            break;
        }
        
        switch (zop)
        {
        case ZOP_UDP_ToServerPacket:
            break;

        case ZOP_UDP_NewClient:
            break;
        
        case ZOP_UDP_ClientLinkdead:
            break;
        
        case ZOP_UDP_ClientDisconnect:
            break;
        
        case ZOP_CS_TerminateThread:
            return;
        
        default:
            log_writef(cs->logQueue, cs->logId, "WARNING: cs_thread_proc: received unexpected zop: %s", enum2str_zop(zop));
            break;
        }
    }
}

CharSelectThread* cs_create(LogThread* log, RingBuf* dbQueue, int dbId, UdpThread* udp)
{
    CharSelectThread* cs = alloc_type(CharSelectThread);
    int rc;
    
    if (!cs) goto fail_alloc;
    
    cs->csQueue = NULL;
    cs->udpQueue = udp_get_queue(udp);
    cs->dbQueue = dbQueue;
    cs->logQueue = log_get_queue(log);
    cs->dbId = dbId;
    
    rc = log_open_file_literal(log, &cs->logId, CHAR_SELECT_THREAD_LOG_PATH);
    if (rc) goto fail;

    cs->csQueue = ringbuf_create_type(ZPacket, 1024);
    if (!cs->csQueue) goto fail;
    
    rc = udp_open_port(udp, CHAR_SELECT_THREAD_UDP_PORT, 0 /*fixme*/, cs->csQueue);
    if (rc) goto fail;

    rc = thread_start(cs_thread_proc, cs);
    if (rc) goto fail;
    
    return cs;
    
fail:
    cs_destroy(cs);
fail_alloc:
    return NULL;
}

CharSelectThread* cs_destroy(CharSelectThread* cs)
{
    if (cs)
    {
        cs->csQueue = ringbuf_destroy(cs->csQueue);
        free(cs);
    }
    
    return NULL;
}

RingBuf* cs_get_queue(CharSelectThread* cs)
{
    return cs->csQueue;
}
