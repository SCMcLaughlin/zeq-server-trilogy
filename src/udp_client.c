
#include "udp_client.h"
#include "aligned.h"
#include "crc.h"
#include "tlg_packet.h"

void udpc_init(UdpClient* udpc, sock_t sock, uint32_t ip, uint16_t port, RingBuf* toServerQueue, RingBuf* logQueue)
{
    udpc->sock = sock;
    udpc->ipAddr.ip = ip;
    udpc->ipAddr.port = port;
    udpc->accountId = 0;
    udpc->clientObject = NULL;
    udpc->toServerQueue = toServerQueue;
    udpc->logQueue = logQueue;

    ack_init(&udpc->ackMgr);
}

bool udpc_recv_protocol(UdpClient* udpc, byte* data, uint32_t len, bool suppressSend)
{
    Aligned a;
    uint16_t header;
    /*uint16_t seq;*/
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
    /*seq = aligned_read_uint16(&a);*/
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
    if (!suppressSend)
    {
        if (ackRequest)
        {
            /* Packet is to be acked, queue it up to ensure it is received in sequence */
            //ack_recv_packet(udpc, &a, opcode, ackRequest, fragCount);
        }
        else if (opcode)
        {
            /* Unsequenced, can be sent on immediately */
            //udpc_recv_packet(udpc, &a, opcode);
        }
    }

    return true;

oob:
    return false;
}

bool udpc_send_immediate(UdpClient* udpc, const void* data, uint32_t len)
{
    IpAddrRaw addr;

    addr.sin_addr.s_addr = udpc->ipAddr.ip;
    addr.sin_port = udpc->ipAddr.port;

    return (0 != sendto(udpc->sock, (const char*)data, len, 0, (struct sockaddr*)&addr, sizeof(addr)));
}

bool udpc_send_pure_ack(UdpClient* udpc, uint16_t ack)
{
    byte buf[10];
    Aligned a;

    aligned_init(&a, buf, sizeof(buf));

    aligned_write_uint16(&a, TLG_PACKET_HasAckResponse);
    aligned_write_uint16(&a, to_network_uint16(udpc->ackMgr.toClient.nextSeqToSend++));
    aligned_write_uint16(&a, ack);
    aligned_write_uint32(&a, crc_calc32_network(buf, sizeof(buf) - sizeof(uint32_t)));

    return udpc_send_immediate(udpc, buf, sizeof(buf));
}
