
#ifndef ZONE_ZPACKET_H
#define ZONE_ZPACKET_H

#include "define.h"
#include "client.h"

typedef struct {
    int     zoneId;
    int     instId;
} Zone_ZCreateZone;

typedef struct {
    int     zoneId;
    int     instId;
    Client* client;
} Zone_ZAddClient;

typedef union {
    Zone_ZCreateZone    zCreateZone;
    Zone_ZAddClient     zAddClient;
} Zone_ZPacket;

#endif/*ZONE_ZPACKET_H*/
