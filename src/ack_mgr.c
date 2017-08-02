
#include "ack_mgr.h"
#include "define_netcode.h"
#include "bit.h"
#include "crc.h"
#include "udp_client.h"
#include "udp_thread.h"
#include "util_alloc.h"
#include "util_clock.h"

#define ACK_RESEND_PACKET_MILLISECONDS_WITHOUT_ACK 250
#define ACK_CMP_WINDOW_HALF_SIZE 1024

enum AckComparison
{
    ACK_Past,
    ACK_Present,
    ACK_Future,
};

/*
    Ack cases:

    1) client->server ack response

        The client gives us an ack that we requested from them. We consider the packet sent with that ack request, and any earlier packets, to be acked and discard them.
        E.g.: the client sends us ack = 5, we discard packets sent with ack = 3, 4, 5, and update our next expected ack response to 6.

    2) client->server ack request

        The client sends us a packet and requests us to use a specific value to ack it.

        Things to consider:

            * Recieving the ack request is decoupled from sending the response. If we are sending a packet to the client during a send cycle, we lump
              the ack they requested into the first packet we send. Otherwise, we expect the keep alive ack to effectively send this ack for us.

            * Ack requests may be received out of order. E.g., the client may send us ack request 1, then 3, then 2. In this case we can send the response
              for 1 immediately, but we cannot send the response for 3 until we have received 2.

    
    Values we need to maintain:
        lastAckReceived - stores the index into our "sent packet" buffer in terms of ack values. Updated when we receive an ack response from the client.
        lastAckSent - the ack response we most recently sent to the client. Updates at the end of a send cycle, if an ack was sent.
        nextAckRequestExpected - one more than the most recent ack request we have received. Updates when a packet is received, after the received packet
            buffer has been driven forward as much as possible (e.g. if there was a backlock of packets that are now in order with the addition of the current packet).
        lowestAckInQueue - stores the start of our received packet buffer in terms of acks.

        If lastAckSent != nextAckRequestExpected - 1, we have an new ack response to send to the client, equal to nextAckRequestExpected - 1.

        When a packet flagged as first/synchronizing is received, we initialize these values with the following, given the input ack:
            lastAckReceived = ? (If this flag is not sent mid-stream, we don't need to init lastAckReceived here. If it is sent mid-stream, we'll need to think more about handling that.)
            lastAckSent = ack - 1
            nextAckRequestExpected = ack + 1
*/

static int ack_cmp(uint16_t got, uint16_t expected)
{
    if (got == expected)
        return ACK_Present;

    if ((got > expected && got < (expected + ACK_CMP_WINDOW_HALF_SIZE)) || got < (expected - ACK_CMP_WINDOW_HALF_SIZE))
        return ACK_Future;

    return ACK_Past;
}

void ack_init(AckMgr* ackMgr)
{
    ackMgr->toServer.nextAckRequestExpected = 0;
    ackMgr->toServer.lastAckSent = 0;
    ackMgr->toServer.lastAckReceived = 0;
    ackMgr->toServer.count = 0;
    ackMgr->toServer.capacity = 0;
    ackMgr->toServer.packetQueue = NULL;
    
    ackMgr->toClient.nextSeqToSend = 0;
    ackMgr->toClient.nextAckToRequest = 0;
    ackMgr->toClient.nextFragGroup = 0;
    ackMgr->toClient.ackCounterAlwaysOne = 1;
    ackMgr->toClient.ackCounterRequest = 0;
    ackMgr->toClient.sendFromIndex = 0;
    ackMgr->toClient.count = 0;
    ackMgr->toClient.packetQueue = NULL;
}

void ack_deinit(AckMgr* ackMgr)
{
    uint32_t n, i;
    
    if (ackMgr->toServer.packetQueue)
    {
        AckMgrToServerPacket* packets = ackMgr->toServer.packetQueue;
        n = ackMgr->toServer.count;
        
        for (i = 0; i < n; i++)
        {
            AckMgrToServerPacket* p = &packets[i];
            
            free_if_exists(p->data);
        }
        
        free(packets);
        ackMgr->toServer.packetQueue = NULL;
    }
    
    if (ackMgr->toClient.packetQueue)
    {
        AckMgrToClientPacket* packets = ackMgr->toClient.packetQueue;
        n = ackMgr->toClient.count;
        
        for (i = 0; i < n; i++)
        {
            packets[i].packet = packet_drop(packets[i].packet);
        }
        
        free(packets);
        ackMgr->toClient.packetQueue = NULL;
    }
}

static void ack_queue_to_client_packet(AckMgr* ackMgr, AckMgrToClientPacket* wrapper)
{
    uint32_t index = ackMgr->toClient.count;
    
    if (bit_is_pow2_or_zero(index))
    {
        uint32_t cap = (index == 0) ? 1 : index * 2;
        AckMgrToClientPacket* queue = realloc_array_type(ackMgr->toClient.packetQueue, cap, AckMgrToClientPacket);
        
        if (!queue) goto error;
        
        ackMgr->toClient.packetQueue = queue;
    }
    
    ackMgr->toClient.packetQueue[index] = *wrapper;
    ackMgr->toClient.count = index + 1;
    return;
    
error:
    packet_drop(wrapper->packet);
}

void ack_schedule_packet(AckMgr* ackMgr, TlgPacket* packet, bool hasAckRequest)
{
    AckMgrToClientPacket wrapper;
    uint16_t header = 0;
    uint16_t fragCount = packet_frag_count(packet);
    
    if (fragCount != 0)
        hasAckRequest = true;
    
    fragCount++;
    
    wrapper.fragCount = fragCount;
    wrapper.packet = packet;
    
    if (hasAckRequest)
    {
        if (ackMgr->toClient.nextAckToRequest == 0)
            ackMgr->toClient.nextAckToRequest = 1; /*fixme: do we even need to bother with this? can we just start from 0?*/
        
        wrapper.ackRequest = ackMgr->toClient.nextAckToRequest;
        ackMgr->toClient.nextAckToRequest += fragCount;
        
        wrapper.ackCounterAlwaysOne = ackMgr->toClient.ackCounterAlwaysOne;
        wrapper.ackCounterRequest = ackMgr->toClient.ackCounterRequest++;
        
        header |= TLG_PACKET_HasAckRequest | TLG_PACKET_HasAckCounter;
        
        if (fragCount > 1)
        {
            header |= TLG_PACKET_IsFragment;
            wrapper.fragGroup = ackMgr->toClient.nextFragGroup++;
        }
    }
    
    wrapper.header = header;
    
    ack_queue_to_client_packet(ackMgr, &wrapper);
}

void ack_recv_ack_response(AckMgr* ackMgr, uint16_t ack)
{
    AckMgrToClientPacket* packets = ackMgr->toClient.packetQueue;
    uint32_t n = ackMgr->toClient.count;
    uint32_t i;

    ack = to_host_uint16(ack);
    ackMgr->toServer.lastAckReceived = ack;

    for (i = 0; i < n; i++)
    {
        AckMgrToClientPacket* packet = &packets[i];
        uint16_t ackRequest = packet->ackRequest + (packet->fragCount - 1);

        if (ack_cmp(ackRequest, ack) == ACK_Future)
            break;

        packet->packet = packet_drop(packet->packet);
    }

    if (i > 0)
    {
        n -= i;
        memmove(packets, &packets[i], sizeof(AckMgrToClientPacket) * n);
        ackMgr->toClient.count = n;
        ackMgr->toClient.sendFromIndex = 0;
    }
}

void ack_recv_sync(AckMgr* ackMgr, uint16_t ackRequest)
{
    ackMgr->toServer.lastAckSent = ackRequest - 1;
    ackMgr->toServer.nextAckRequestExpected = ackRequest;
}

static void ack_complete_packet(UdpThread* udp, UdpClient* udpc, AckMgrToServerPacket* packet)
{
    Aligned a;
    
    aligned_init(&a, packet->data, packet->length);
    udpc_recv_packet_no_copy(udp, udpc, &a, packet->opcode);
}

static void ack_complete_fragments(UdpThread* udp, UdpClient* udpc, AckMgrToServerPacket* array, uint32_t length, uint32_t i, uint32_t n)
{
    AckMgrToServerPacket* packet = NULL;
    Aligned a;
    uint16_t opcode = array[i].opcode;
    byte* data = alloc_bytes(length);
    
    if (!data) return;
    
    aligned_init(&a, data, length);
    
    for (; i <= n; i++)
    {
        packet = &array[i];
        
        aligned_write_buffer(&a, packet->data, packet->length);
        free(packet->data);
        packet->data = NULL;
    }
    
    aligned_reset_cursor(&a);
    udpc_recv_packet_no_copy(udp, udpc, &a, opcode);
}

#if 0
static void ack_debug_print_queue(AckMgr* ackMgr)
{
    uint32_t i;

    for (i = 0; i < ackMgr->toServer.capacity; i++)
    {
        AckMgrToServerPacket* p = &ackMgr->toServer.packetQueue[i];

        printf("[%2i] ack: %04x, op: %04x, fragCount: %u, len: %u\n", i, p->ackRequest, p->opcode, p->fragCount, p->length);
    }
}
#endif

void ack_recv_packet(UdpThread* udp, UdpClient* udpc, Aligned* a, uint16_t opcode, uint16_t ackRequest, uint16_t fragCount)
{
    AckMgr* ackMgr = &udpc->ackMgr;
    uint16_t nextAck = ackMgr->toServer.nextAckRequestExpected;
    uint32_t n, i, diff, length, frags;
    AckMgrToServerPacket* ptr;

    if (ackRequest == nextAck && fragCount == 0)
    {
        nextAck++;
        ackMgr->toServer.nextAckRequestExpected = nextAck;
        ackMgr->toServer.lowestAckInQueue = nextAck;
        udpc_recv_packet(udp, udpc, a, opcode);
        return;
    }
    
    if (ack_cmp(ackRequest, nextAck) == ACK_Past)
        return;

    n = ackMgr->toServer.count;
    
    /* Protocol doesn't allow acks to roll over to 0, so shouldn't need to check for ackRequest being lower (fixme: confirm this) */
    diff = ackRequest - ackMgr->toServer.lowestAckInQueue;
    
    /* Do we need to expand our array? */
    if (diff >= ackMgr->toServer.capacity)
    {
        uint32_t cap;
        uint32_t d = diff;
        AckMgrToServerPacket* queue;

        if (d == 0)
            d = 1;

        cap = bit_pow2_greater_than_u32(d);
        queue = realloc_array_type(ackMgr->toServer.packetQueue, cap, AckMgrToServerPacket);

        if (!queue) return;

        ackMgr->toServer.packetQueue = queue;
        ackMgr->toServer.capacity = cap;
        memset(&ackMgr->toServer.packetQueue[n], 0, sizeof(AckMgrToServerPacket) * (cap - n));
    }
    
    ptr = &ackMgr->toServer.packetQueue[diff];
    
    /*
        If we already had a packet at this position, abort.
        The client likes to spam the same packet over and over in some situations, especially during login
    */
    if (ptr->ackRequest)
        return;
    
    length = aligned_remaining_data(a);
    ptr->ackRequest = ackRequest;
    ptr->fragCount = fragCount;
    ptr->opcode = opcode;
    ptr->length = length;
    
    if (length)
    {
        ptr->data = alloc_bytes(length);
        if (!ptr->data) return;
        memcpy(ptr->data, aligned_current(a), length);
    }
    else
    {
        ptr->data = NULL;
    }
    
    /* Check if we have completed any fragmented packets and such */
    ptr = ackMgr->toServer.packetQueue;
    diff = 0;
    frags = 0;
    length = 0;
    i = 0;

    n++; /* Include the packet we just added */
    
    while (i < n)
    {
        AckMgrToServerPacket* packet = &ptr[i];
        
        if (packet->ackRequest == 0)
            break;
        
        fragCount = packet->fragCount;
        
        if (fragCount == 0)
        {
            ack_complete_packet(udp, udpc, packet);
            nextAck = packet->ackRequest;
            ackMgr->toServer.lowestAckInQueue = nextAck + 1;
            diff++;
        }
        else
        {
            if (fragCount > n)
                break;
            
            length += packet->length;
            frags++;
            nextAck = packet->ackRequest;
            
            if (frags == fragCount)
            {
                diff += frags;
                ack_complete_fragments(udp, udpc, ptr, length, i - (frags - 1), i);
                ackMgr->toServer.lowestAckInQueue = nextAck + 1;
                length = 0;
                frags = 0;
            }
        }
        
        i++;
    }
    
    if (diff > 0)
    {
        uint32_t count = n - diff;
        ackMgr->toServer.count = count;

        if (count)
            memmove(ackMgr->toServer.packetQueue, &ackMgr->toServer.packetQueue[diff], sizeof(AckMgrToServerPacket) * count);

        diff = n - count;

        if (diff)
            memset(&ackMgr->toServer.packetQueue[count], 0, sizeof(AckMgrToServerPacket) * diff);
    }
    else
    {
        ackMgr->toServer.count = n;
    }

    ackMgr->toServer.nextAckRequestExpected = nextAck +1;
}

static void ack_send_fragment(UdpClient* udpc, AckMgr* ackMgr, AckMgrToClientPacket* wrapper, Aligned* a, uint32_t dataLength, uint16_t opcode, bool hasAckResponse, uint16_t ackResponse, uint16_t fragIndex)
{
    uint32_t orig = aligned_position(a);
    uint16_t header = wrapper->header;
    byte* data;
    
    /* opcode */
    if (fragIndex == 0 && opcode)
        aligned_write_reverse_uint16(a, opcode);
    
    /* ack counters */
    if (header & TLG_PACKET_HasAckRequest)
    {
        aligned_write_reverse_uint8(a, wrapper->ackCounterRequest + fragIndex);
        aligned_write_reverse_uint8(a, wrapper->ackCounterAlwaysOne);
    }
    
    /* fragment info */
    if (header & TLG_PACKET_IsFragment)
    {
        aligned_write_reverse_uint16(a, to_network_uint16(wrapper->fragCount));
        aligned_write_reverse_uint16(a, to_network_uint16(fragIndex));
        aligned_write_reverse_uint16(a, to_network_uint16(wrapper->fragGroup));
    }
    
    /* ackRequest */
    if (header & TLG_PACKET_HasAckRequest)
        aligned_write_reverse_uint16(a, to_network_uint16(wrapper->ackRequest + fragIndex));
    
    /* ackResponse */
    if (hasAckResponse)
    {
        header |= TLG_PACKET_HasAckResponse;
        aligned_write_reverse_uint16(a, ackResponse);
    }
    
    /* sequence */
    aligned_write_reverse_uint16(a, ack_get_next_seq_to_send_and_increment(ackMgr));
    
    /* header */
    aligned_write_reverse_uint16(a, header);
    
    /* crc */
    data = aligned_current(a);
    dataLength += orig - aligned_position(a);
    
    aligned_advance_by(a, dataLength);
    aligned_write_uint32(a, crc_calc32_network(data, dataLength));
    
    udpc_send_immediate(udpc, data, dataLength + sizeof(uint32_t));
}

void ack_send_queued_packets(UdpThread* udp, UdpClient* udpc)
{
    AckMgr* ackMgr = &udpc->ackMgr;
    AckMgrToClientPacket* packets = ackMgr->toClient.packetQueue;
    uint32_t n = ackMgr->toClient.count;
    uint16_t nextAck = ackMgr->toServer.nextAckRequestExpected -1;
    uint16_t ackReceived = ackMgr->toServer.lastAckReceived;
    uint32_t sendFromIndex = ackMgr->toClient.sendFromIndex;
    uint64_t time = clock_milliseconds();
    int hasAckResponse = 0;
    uint16_t ackResponse = 0;
    Aligned a;
    uint32_t i;

    if (ackMgr->toServer.lastAckSent != nextAck)
    {
        hasAckResponse = 1;
        ackResponse = to_network_uint16(nextAck);
    }
    
    for (i = 0; i < n; i++)
    {
        AckMgrToClientPacket* wrapper = &packets[i];
        TlgPacket* packet = wrapper->packet;
        uint32_t dataLength = packet_length(packet);
        uint16_t opcode = packet_opcode(packet);
        uint16_t fragCount = wrapper->fragCount;
        uint16_t fragIndex = 0;
        
        aligned_init(&a, packet_buffer_raw(packet), packet_buffer_raw_length(packet));
        
        if (i < sendFromIndex)
        {
            /* Should we re-send this un-acked packet? */
            if ((time - wrapper->ackTimestamp) < ACK_RESEND_PACKET_MILLISECONDS_WITHOUT_ACK)
                continue;
            
            /* Figure out where to re-send from if we were last acked in the middle of a fragmented packet */
            if (fragCount > 1)
            {
                uint16_t ack = wrapper->ackRequest;
                
                while (ack_cmp(ackReceived, ack) != ACK_Past && fragIndex < fragCount)
                {
                    aligned_advance_by(&a, TLG_PACKET_DATA_OFFSET + TLG_PACKET_DATA_SPACE - sizeof(uint16_t) + sizeof(uint32_t));
                    
                    if (fragIndex > 0)
                        aligned_advance_by(&a, sizeof(uint16_t));
                    
                    fragIndex++;
                    ack++;
                }
            }
        }
        
        /* Do our sends, potentially starting mid-fragmented packet */
        for (; fragIndex < fragCount; fragIndex++)
        {
            uint32_t space = TLG_PACKET_DATA_SPACE;
            bool hasAck = (hasAckResponse == 1);
            
            if (fragIndex == 0)
                space -= sizeof(uint16_t);
            
            aligned_advance_by(&a, TLG_PACKET_DATA_OFFSET);
            
            ack_send_fragment(udpc, ackMgr, wrapper, &a, (dataLength > space) ? space : dataLength, opcode, hasAck, ackResponse, fragIndex);
            
            wrapper->ackTimestamp = clock_milliseconds();
            
            if (hasAck)
            {
                udpc_flag_last_ack(udp, udpc);
                hasAckResponse = 2;
            }

            dataLength -= space;
        }
    }
    
    if (hasAckResponse == 1)
        udpc_send_pure_ack(udp, udpc, ackResponse);
    
    if (hasAckResponse != 0)
        ackMgr->toServer.lastAckSent = nextAck;

    ackMgr->toClient.sendFromIndex = i;
}

uint16_t ack_get_next_seq_to_send_and_increment(AckMgr* ackMgr)
{
    return to_network_uint16(ackMgr->toClient.nextSeqToSend++);
}

uint16_t ack_get_keep_alive_ack(AckMgr* ackMgr)
{
    uint16_t nextAck = ackMgr->toServer.nextAckRequestExpected - 1;
    uint16_t ackToSend = ackMgr->toServer.lastAckSent;

    if (ackToSend != nextAck)
    {
        ackToSend = nextAck;
        ackMgr->toServer.lastAckSent = nextAck;
    }

    return to_network_uint16(ackToSend);
}
