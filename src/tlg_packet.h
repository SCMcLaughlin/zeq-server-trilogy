
#ifndef TLG_PACKET_H
#define TLG_PACKET_H

#include "define.h"

#define TLG_PACKET_HEADER_SIZE (sizeof(uint16_t) * 9)
#define TLG_PACKET_DATA_OFFSET TLG_PACKET_HEADER_SIZE
#define TLG_PACKET_DATA_SPACE 512
#define TLG_PACKET_CRC_SIZE sizeof(uint32_t)

typedef struct TlgPacket TlgPacket;

enum TlgPacketHeaderBits
{
    TLG_PACKET_UnknownBit0      = 0x0001,
    TLG_PACKET_HasAckRequest    = 0x0002,
    TLG_PACKET_IsClosing        = 0x0004,
    TLG_PACKET_IsFragment       = 0x0008,
    TLG_PACKET_HasAckCounter    = 0x0010,
    TLG_PACKET_IsFirstPacket    = 0x0020,
    TLG_PACKET_IsClosing2       = 0x0040,
    TLG_PACKET_IsSequenceEnd    = 0x0080,
    TLG_PACKET_IsKeepAliveAck   = 0x0100,
    TLG_PACKET_UnknownBit9      = 0x0200,
    TLG_PACKET_HasAckResponse   = 0x0400,
    TLG_PACKET_UnknownBit11     = 0x0800,
    TLG_PACKET_UnknownBit12     = 0x1000,
    TLG_PACKET_UnknownBit13     = 0x2000,
    TLG_PACKET_UnknownBit14     = 0x4000,
    TLG_PACKET_UnknownBit15     = 0x8000,
};

TlgPacket* packet_create(uint16_t opcode, uint32_t length);
#define packet_create_type(opcode, type) packet_create((opcode), sizeof(type))
#define packet_create_opcode_only(opcode) packet_create((opcode), 0)
TlgPacket* packet_drop(TlgPacket* packet);
void packet_grab(TlgPacket* packet);

uint16_t packet_opcode(TlgPacket* packet);
byte* packet_data(TlgPacket* packet);
byte* packet_buffer_raw(TlgPacket* packet);
uint32_t packet_length(TlgPacket* packet);
uint32_t packet_buffer_raw_length(TlgPacket* packet);
uint32_t packet_frag_count(TlgPacket* packet);

bool packet_already_fragmentized(TlgPacket* packet);
void packet_fragmentize(TlgPacket* packet);

#endif/*TLG_PACKET_H*/
