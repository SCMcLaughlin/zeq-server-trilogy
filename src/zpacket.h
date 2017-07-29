
#ifndef ZPACKET_H
#define ZPACKET_H

#include "db_zpacket.h"
#include "udp_zpacket.h"

typedef union {
    DB_ZPacket  db;
    UDP_ZPacket udp;
} ZPacket;

#endif/*ZPACKET_H*/
