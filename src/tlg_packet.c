
#include "tlg_packet.h"
#include "define_netcode.h"
#include "crc.h"
#include "aligned.h"
#include "util_alloc.h"
#include "util_atomic.h"

#ifdef PLATFORM_WINDOWS
/* data[0]: "nonstandard extension used: zero-sized array in struct/union" */
# pragma warning(disable: 4200)
#endif

#define TLG_PACKET_HEADER_SIZE (sizeof(uint16_t) * 9)
#define TLG_PACKET_DATA_OFFSET TLG_PACKET_HEADER_SIZE
#define TLG_PACKET_DATA_SPACE 512

/*
The full packet header looks something like this. We work backwards from the opcode;
all fields are filled in at the protocol level, just before sending-time

struct CompleteHeader {
    uint16_t    header;
    uint16_t    sequence;
    uint16_t    ackResponse;
    uint16_t    ackRequest;
    uint16_t    fragIndex;
    uint16_t    fragGroup;
    uint16_t    fragsInGroup;
    uint8_t     counterHigh;
    uint8_t     counterLow;
    uint16_t    opcode;
};
*/

struct TlgPacket {
    uint16_t    opcode;
    uint16_t    fragCount;
    uint32_t    dataLength;
    atomic32_t  refCount;
    byte        data[0];
};

static uint32_t packet_calc_length_and_frag_count(uint32_t length, uint16_t* fragCount)
{
    if (length <= TLG_PACKET_DATA_SPACE)
    {
        *fragCount = 0;
        return sizeof(TlgPacket) + TLG_PACKET_HEADER_SIZE + length + TLG_PACKET_CRC_SIZE;
    }
    else
    {
        uint32_t count = length / TLG_PACKET_DATA_SPACE;
        uint32_t rem = length % TLG_PACKET_DATA_SPACE;

        if (rem)
            count++;

        /*
            The idea here is to leave enough space in our buffer to spread out all the fragments
            in-place, with large enough caps to allow for the largest possible variable-sized header
            as well as the CRC footer, in each fragment.
        */

        *fragCount = count - 1;
        return sizeof(TlgPacket) + ((TLG_PACKET_HEADER_SIZE + TLG_PACKET_CRC_SIZE) * count) + length;
    }
}

TlgPacket* packet_create(uint16_t opcode, uint32_t length)
{
    TlgPacket* packet;
    uint16_t fragCount;
    uint32_t allocLength = packet_calc_length_and_frag_count(length, &fragCount);

    packet = alloc_bytes_type(allocLength, TlgPacket);

    if (!packet) return NULL;

    packet->opcode = opcode;
    packet->fragCount = fragCount;
    packet->dataLength = length;
    atomic32_set(&packet->refCount, 0);

    return packet;
}

TlgPacket* packet_drop(TlgPacket* packet)
{
    if (packet && atomic32_sub(&packet->refCount, 1) <= 1)
    {
        free(packet);
    }

    return NULL;
}

void packet_grab(TlgPacket* packet)
{
    atomic32_add(&packet->refCount, 1);
}

uint16_t packet_opcode(TlgPacket* packet)
{
    return packet->opcode;
}

byte* packet_data(TlgPacket* packet)
{
    return &packet->data[TLG_PACKET_DATA_OFFSET];
}

byte* packet_buffer_raw(TlgPacket* packet)
{
    return &packet->data[0];
}

uint32_t packet_length(TlgPacket* packet)
{
    return packet->dataLength;
}

uint32_t packet_buffer_raw_length(TlgPacket* packet)
{
    return packet->dataLength + ((TLG_PACKET_DATA_OFFSET + TLG_PACKET_CRC_SIZE) * (packet->fragCount + 1));
}

uint32_t packet_frag_count(TlgPacket* packet)
{
    return packet->fragCount;
}

void packet_fragmentize(TlgPacket* packet)
{
    uint16_t n = packet->fragCount;
    uint32_t dataLength;
    uint32_t dst;
    uint32_t src;
    uint16_t i;
    uint32_t j;
    byte* data;

    if (n == 0) return;

    dataLength = packet->dataLength;
    src = dataLength + TLG_PACKET_DATA_OFFSET - 1;
    dst = packet_buffer_raw_length(packet) - TLG_PACKET_CRC_SIZE - 1;
    data = packet->data;

    /* 
        The opcode counts towards the size of the first fragment; any non-final fragment
        that doesn't make use of its full size will be rejected by the client. We add
        the size of the opcode here so that 2 extra bytes will go towards the final
        packet, effectively shifting everything over without increasing the 'actual'
        data size. (Yes, it is a bit confusing.)
    */
    dataLength += sizeof(uint16_t);
    dataLength %= TLG_PACKET_DATA_SPACE;

    if (dataLength == 0)
        dataLength = TLG_PACKET_DATA_SPACE;

    for (i = 0; i < n; i++)
    {
        for (j = 0; j < dataLength; j++)
        {
            data[dst--] = data[src--];
        }

        dst -= TLG_PACKET_DATA_OFFSET + TLG_PACKET_CRC_SIZE;
        dataLength = TLG_PACKET_DATA_SPACE;
    }
}
