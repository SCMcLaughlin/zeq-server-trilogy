
#ifndef ZPACKET_H
#define ZPACKET_H

#include "char_select_zpacket.h"
#include "db_zpacket.h"
#include "login_zpacket.h"
#include "main_zpacket.h"
#include "udp_zpacket.h"

typedef union {
    CharSelect_ZPacket  cs;
    DB_ZPacket          db;
    Login_ZPacket       login;
    Main_ZPacket        main;
    UDP_ZPacket         udp;
} ZPacket;

#endif/*ZPACKET_H*/
