
#include "char_select_thread.h"
#include "char_select_client.h"
#include "tlg_packet.h"
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
    int         dbId;
    int         logId;
    RingBuf*    csQueue;
    RingBuf*    udpQueue;
    RingBuf*    dbQueue;
    RingBuf*    logQueue;
    /* Cached packets that are more or less static for all clients */
    TlgPacket*  packetEcho1;
    TlgPacket*  packetEcho2;
    TlgPacket*  packetEcho3;
    TlgPacket*  packetEcho4;
    TlgPacket*  packetEcho5;
};

static bool login_thread_drop_client(CharSelectThread* cs, CharSelectClient* client)
{
    ZPacket cmd;
    int rc;
    
    cmd.udp.zDropClient.ipAddress = csc_get_ip_addr(client);
    cmd.udp.zDropClient.packet = NULL;
    
    rc = ringbuf_push(cs->udpQueue, ZOP_UDP_DropClient, &cmd);
    
    if (rc)
    {
        uint32_t ip = cmd.udp.zDropClient.ipAddress.ip;
        
        log_writef(cs->logQueue, cs->logId, "cs_thread_drop_client: failed to inform UdpThread to drop client from %u.%u.%u.%u:%u, keeping client alive for now",
            (ip >> 0) & 0xff, (ip >> 8) & 0xff, (ip >> 16) & 0xff, (ip >> 24) & 0xff, cmd.udp.zDropClient.ipAddress.port);
    
        return false;
    }
    
    return true;
}

static int cs_thread_handle_op_echo(CharSelectThread* cs, CharSelectClient* client, uint16_t opcode)
{
    TlgPacket* packet;
    
    switch (opcode)
    {
    case OP_CS_Echo1:
        packet = cs->packetEcho1;
        break;
    
    case OP_CS_Echo2:
        packet = cs->packetEcho2;
        break;
    
    case OP_CS_Echo3:
        packet = cs->packetEcho3;
        break;
    
    case OP_CS_Echo4:
        packet = cs->packetEcho4;
        break;
    
    case OP_CS_Echo5:
        packet = cs->packetEcho5;
        break;
    }
    
    return csc_schedule_packet(client, cs->udpQueue, packet);
}

static void cs_thread_handle_packet(CharSelectThread* cs, ZPacket* zpacket)
{
    CharSelectClient* client = (CharSelectClient*)zpacket->udp.zToServerPacket.clientObject;
    byte* data = zpacket->udp.zToServerPacket.data;
    uint16_t opcode = zpacket->udp.zToServerPacket.opcode;
    int rc = ERR_None;
    
    switch (opcode)
    {
    case OP_CS_Echo1:
    case OP_CS_Echo2:
    case OP_CS_Echo3:
    case OP_CS_Echo4:
    case OP_CS_Echo5:
        rc = cs_thread_handle_op_echo(cs, client, opcode);
        break;
    
    default:
        log_writef(cs->logQueue, cs->logId, "WARNING: cs_thread_handle_packet: received unexpected char select opcode: %04x", zpacket->udp.zToServerPacket.opcode);
        break;
    }
    
    if (rc)
    {
        uint32_t ip = csc_get_ip(client);
        uint16_t port = csc_get_port(client);
        
        log_writef(cs->logQueue, cs->logId, "cs_thread_handle_packet: got error %s while processing packet with opcode %s from client from %u.%u.%u.%u:%u, disconnecting them to maintain consistency",
            enum2str_err(rc), enum2str_char_select_opcode(opcode), (ip >> 0) & 0xff, (ip >> 8) & 0xff, (ip >> 16) & 0xff, (ip >> 24) & 0xff, port);
        
        login_thread_drop_client(cs, client);
    }
    
    if (data) free(data);
}

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
            cs_thread_handle_packet(cs, &zpacket);
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

static int cs_create_cached_packets(CharSelectThread* cs)
{
    /* Zero-length echo packets */
    cs->packetEcho1 = packet_create_opcode_only(OP_CS_Echo1);
    if (!cs->packetEcho1) goto fail;
    packet_grab(cs->packetEcho1);
    
    cs->packetEcho2 = packet_create_opcode_only(OP_CS_Echo2);
    if (!cs->packetEcho2) goto fail;
    packet_grab(cs->packetEcho2);
    
    cs->packetEcho3 = packet_create_opcode_only(OP_CS_Echo3);
    if (!cs->packetEcho3) goto fail;
    packet_grab(cs->packetEcho3);
    
    cs->packetEcho4 = packet_create_opcode_only(OP_CS_Echo4);
    if (!cs->packetEcho4) goto fail;
    packet_grab(cs->packetEcho4);
    
    cs->packetEcho5 = packet_create_opcode_only(OP_CS_Echo5);
    if (!cs->packetEcho5) goto fail;
    packet_grab(cs->packetEcho5);
    
    return ERR_None;
fail:
    return ERR_OutOfMemory;
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
    
    rc = cs_create_cached_packets(cs);
    if (rc) goto fail;
    
    rc = udp_open_port(udp, CHAR_SELECT_THREAD_UDP_PORT, csc_sizeof(), cs->csQueue);
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
        
        cs->packetEcho1 = packet_drop(cs->packetEcho1);
        cs->packetEcho2 = packet_drop(cs->packetEcho2);
        cs->packetEcho3 = packet_drop(cs->packetEcho3);
        cs->packetEcho4 = packet_drop(cs->packetEcho4);
        cs->packetEcho5 = packet_drop(cs->packetEcho5);
        
        free(cs);
    }
    
    return NULL;
}

RingBuf* cs_get_queue(CharSelectThread* cs)
{
    return cs->csQueue;
}
