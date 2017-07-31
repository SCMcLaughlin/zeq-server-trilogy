
#include "udp_client.h"
#include "aligned.h"
#include "crc.h"
#include "log_thread.h"
#include "tlg_packet.h"
#include "util_alloc.h"
#include "zpacket.h"
#include "enum_zop.h"
#include "enum2str.h"

void udpc_init(UdpClient* udpc, sock_t sock, uint32_t ip, uint16_t port, RingBuf* toServerQueue, RingBuf* logQueue, int logId)
{
    udpc->sock = sock;
    udpc->ipAddr.ip = ip;
    udpc->ipAddr.port = port;
    udpc->accountId = 0;
    udpc->logId = logId;
    udpc->clientObject = NULL;
    udpc->toServerQueue = toServerQueue;
    udpc->logQueue = logQueue;

    ack_init(&udpc->ackMgr);
}

static void udpc_report_disconnect(UdpClient* udpc, int zop)
{
    ZPacket zpacket;
    int rc;
    
    zpacket.udp.zClientDisconnect.clientObject = udpc->clientObject;
    rc = ringbuf_push(udpc->toServerQueue, zop, &zpacket);
    
    if (rc)
    {
        log_writef(udpc->logQueue, udpc->logId, "udpc_report_disconnect: ringbuf_push() failed for zop %s with error %s", enum2str_zop(zop), enum2str_err(rc));
    }
}

void udpc_deinit(UdpClient* udpc)
{
    ack_deinit(&udpc->ackMgr);
}

void udpc_linkdead(UdpClient* udpc)
{
    uint32_t ip = udpc->ipAddr.ip;
    uint16_t port = udpc->ipAddr.port;
    
    log_writef(udpc->logQueue, udpc->logId, "UDP client from %u.%u.%u.%u:%u is LINKDEAD",
        (ip >> 0) & 0xff, (ip >> 8) & 0xff, (ip >> 16) & 0xff, (ip >> 24) & 0xff, port);
    
    udpc_report_disconnect(udpc, ZOP_UDP_ClientLinkdead);
}

bool udpc_send_disconnect(UdpClient* udpc)
{
    typedef struct {
        uint16_t    header;
        uint16_t    seq;
        uint32_t    crc;
    } PS_Disconnect;
    
    PS_Disconnect dis;
    uint32_t ip = udpc->ipAddr.ip;
    uint16_t port = udpc->ipAddr.port;
    
    assert(sizeof(dis) == 8);
    
    dis.header = TLG_PACKET_IsClosing | TLG_PACKET_IsClosing2;
    dis.seq = ack_get_next_seq_to_send_and_increment(&udpc->ackMgr);
    dis.crc = crc_calc32_network(&dis, sizeof(dis));
    
    log_writef(udpc->logQueue, udpc->logId, "Disconnecting UDP client from %u.%u.%u.%u:%u",
        (ip >> 0) & 0xff, (ip >> 8) & 0xff, (ip >> 16) & 0xff, (ip >> 24) & 0xff, port);
    
    udpc_report_disconnect(udpc, ZOP_UDP_ClientDisconnect);
    
    return udpc_send_immediate(udpc, &dis, sizeof(dis));
}

void udpc_recv_packet(UdpClient* udpc, Aligned* a, uint16_t opcode)
{
    Aligned w;
    byte* data;
    uint32_t length;

    if (opcode == 0) return;

    length = aligned_remaining_data(a);
    
    if (length)
    {
        data = alloc_bytes(length);
        
        if (!data)
        {
            log_writef(udpc->logQueue, udpc->logId, "udpc_recv_packet: allocation failed for packet data with length %u", length);
            return;
        }
        
        memcpy(data, aligned_current(a), length);
    }
    else
    {
        data = NULL;
    }
    
    aligned_init(&w, data, length);
    udpc_recv_packet_no_copy(udpc, &w, opcode);
}

void udpc_recv_packet_no_copy(UdpClient* udpc, Aligned* a, uint16_t opcode)
{
    ZPacket zpacket;
    uint32_t length = aligned_remaining_data(a);
    byte* data = aligned_current(a);
    int rc;
    
    zpacket.udp.zToServerPacket.opcode = opcode;
    zpacket.udp.zToServerPacket.length = length;
    zpacket.udp.zToServerPacket.clientObject = udpc->clientObject;
    zpacket.udp.zToServerPacket.data = data;
    
    rc = ringbuf_push(udpc->toServerQueue, ZOP_UDP_ToServerPacket, &zpacket);
    
    if (rc)
    {
        log_writef(udpc->logQueue, udpc->logId, "udpc_recv_packet_no_copy: ringbuf_push() failed, error: %s", enum2str_err(rc));
        if (data) free(data);
    }
}

bool udpc_recv_protocol(UdpClient* udpc, byte* data, uint32_t len, bool suppressRecv)
{
    Aligned a;
    uint16_t header;
    uint16_t opcode = 0;
    uint16_t ackRequest = 0;
    uint16_t fragCount = 0;
    uint16_t fragIndex = 0;
    
    aligned_init(&a, data, len);

    /* Every trilogy packet has a crc, check it first */
    len -= TLG_PACKET_CRC_SIZE;
    aligned_advance_by(&a, len);

    if (crc_calc32_network(data, len) != aligned_read_uint32(&a))
        return false;

    aligned_reset_cursor(&a);
    aligned_set_length(&a, len);

    /*
        The caller guarantees that any packet passed to this function is at least 10 bytes.
        The CRC accounts for 4, so we know we have at least 6 remaining.
    */
    header = aligned_read_uint16(&a);
    aligned_advance_by_sizeof(&a, uint16_t); /* sequence */

    if (header & TLG_PACKET_HasAckResponse)
    {
        uint16_t ackResponse = aligned_read_uint16(&a);
        ack_recv_ack_response(&udpc->ackMgr, ackResponse);

        /* Was this a pure ack packet? If so, we are done handling it */
        if (aligned_remaining_data(&a) == 0)
            return true;
    }

    /* Weird, rare fields we don't care about, but need to skip if present */
    if (header & TLG_PACKET_UnknownBit11)
    {
        if (aligned_remaining_data(&a) < sizeof(uint16_t))
            goto oob;
        aligned_advance_by_sizeof(&a, uint16_t);
    }

    if (header & TLG_PACKET_UnknownBit12)
    {
        if (aligned_remaining_data(&a) < sizeof(byte))
            goto oob;
        aligned_advance_by_sizeof(&a, byte);
    }

    if (header & TLG_PACKET_UnknownBit13)
    {
        if (aligned_remaining_data(&a) < sizeof(uint16_t))
            goto oob;
        aligned_advance_by_sizeof(&a, uint16_t);
    }

    if (header & TLG_PACKET_UnknownBit14)
    {
        if (aligned_remaining_data(&a) < sizeof(uint32_t))
            goto oob;
        aligned_advance_by_sizeof(&a, uint32_t);
    }

    if (header & TLG_PACKET_UnknownBit15)
    {
        if (aligned_remaining_data(&a) < sizeof(uint64_t))
            goto oob;
        aligned_advance_by_sizeof(&a, uint64_t);
    }

    /* Ack request */
    if (header & TLG_PACKET_HasAckRequest)
    {
        if (aligned_remaining_data(&a) < sizeof(uint16_t))
            goto oob;

        ackRequest = to_host_uint16(aligned_read_uint16(&a));
        ack_recv_ack_request(&udpc->ackMgr, ackRequest, ((header & TLG_PACKET_IsFirstPacket) != 0));
    }

    /* Session finalizer */
    if (header & (TLG_PACKET_IsClosing | TLG_PACKET_IsClosing2))
    {
        udpc_send_pure_ack(udpc, to_network_uint16(ackRequest));
        //flag_connection_as_dead(udpc);
        //client_on_disconnect(udpc);
        return false;
    }

    /* Fragment tracking */
    if (header & TLG_PACKET_IsFragment)
    {
        if (aligned_remaining_data(&a) < (sizeof(uint16_t) * 3))
            goto oob;

        aligned_advance_by_sizeof(&a, uint16_t); /* fragGroup */
        fragIndex = to_host_uint16(aligned_read_uint16(&a));
        fragCount = to_host_uint16(aligned_read_uint16(&a));
    }

    /* Counters we don't care about */
    if (header & TLG_PACKET_HasAckCounter)
    {
        uint32_t size = (header & TLG_PACKET_HasAckRequest) ? sizeof(uint16_t) : sizeof(uint8_t);

        if (aligned_remaining_data(&a) < size)
            goto oob;
        aligned_advance_by(&a, size);
    }

    /* Opcode */
    if (fragIndex == 0 && aligned_remaining_data(&a) >= sizeof(uint16_t))
    {
        opcode = aligned_read_uint16(&a);
    }

    /* Packet processing */
    if (!suppressRecv)
    {
        if (ackRequest)
        {
            /* Packet is to be acked, queue it up to ensure it is received in sequence */
            ack_recv_packet(udpc, &a, opcode, ackRequest, fragCount);
        }
        else if (opcode)
        {
            /* Unsequenced, can be sent on immediately */
            udpc_recv_packet(udpc, &a, opcode);
        }
    }

    return true;

oob:
    return false;
}

void udpc_schedule_packet(UdpClient* udpc, TlgPacket* packet, bool hasAckRequest)
{
    ack_schedule_packet(&udpc->ackMgr, packet, hasAckRequest);
}

static void udpc_dump_packet_send(const void* ptr, uint32_t len)
{
#ifndef ZEQ_UDP_DUMP_PACKETS
    (void)ptr;
    (void)len;
#else
    const byte* data = (const byte*)ptr;
    uint32_t i;
    
    fprintf(stdout, "[Send] (%u):\n", len);
    
    for (i = 0; i < len; i++)
    {
        fprintf(stdout, "%02x ", data[i]);
    }
    
    fputc('\n', stdout);
    fflush(stdout);
#endif
}

bool udpc_send_immediate(UdpClient* udpc, const void* data, uint32_t len)
{
    IpAddrRaw addr;

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = udpc->ipAddr.ip;
    addr.sin_port = udpc->ipAddr.port;
    
    udpc_dump_packet_send(data, len);

    return (0 != sendto(udpc->sock, (const char*)data, len, 0, (struct sockaddr*)&addr, sizeof(addr)));
}

void udpc_send_queued_packets(UdpClient* udpc)
{
    ack_send_queued_packets(udpc);
}

bool udpc_send_pure_ack(UdpClient* udpc, uint16_t ackNetworkByteOrder)
{
    byte buf[10];
    Aligned a;

    aligned_init(&a, buf, sizeof(buf));

    aligned_write_uint16(&a, TLG_PACKET_HasAckResponse);
    aligned_write_uint16(&a, to_network_uint16(udpc->ackMgr.toClient.nextSeqToSend++));
    aligned_write_uint16(&a, ackNetworkByteOrder);
    aligned_write_uint32(&a, crc_calc32_network(buf, sizeof(buf) - sizeof(uint32_t)));

    return udpc_send_immediate(udpc, buf, sizeof(buf));
}
