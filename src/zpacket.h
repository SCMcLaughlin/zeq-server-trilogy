
#ifndef ZPACKET_H
#define ZPACKET_H

#include "db_zpacket.h"
#include "login_zpacket.h"
#include "udp_zpacket.h"

typedef union {
    DB_ZPacket      db;
    Login_ZPacket   login;
    UDP_ZPacket     udp;
} ZPacket;

#endif/*ZPACKET_H*/
