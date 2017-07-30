
#ifndef UDP_ZPACKET_H
#define UDP_ZPACKET_H

#include "define.h"
#include "define_netcode.h"
#include "ringbuf.h"
#include "tlg_packet.h"

typedef struct {
    uint16_t    opcode;
    uint32_t    length;
    byte*       data;
} ToServerPacket;

typedef struct {
    uint16_t    port;
    uint32_t    clientSize;
    RingBuf*    toServerQueue;
} UDP_ZOpenPort;

typedef struct {
    void*   clientObject;
    IpAddr  ipAddress;
} UDP_ZNewClient;

typedef struct {
    uint16_t    opcode;
    uint32_t    length;
    void*       clientObject;
    byte*       data;
} UDP_ZToServerPacket;

typedef struct {
    IpAddr      ipAddress;
    TlgPacket*  packet;
} UDP_ZToClientPacket;

typedef union {
    UDP_ZOpenPort       zOpenPort;
    UDP_ZNewClient      zNewClient;
    UDP_ZToClientPacket zDropClient;
    UDP_ZToServerPacket zToServerPacket;
    UDP_ZToClientPacket zToClientPacket;
} UDP_ZPacket;

#endif/*UDP_ZPACKET_H*/
