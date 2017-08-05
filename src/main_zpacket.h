
#ifndef MAIN_ZPACKET_H
#define MAIN_ZPACKET_H

#include "define.h"
#include "buffer.h"

typedef struct {
    void*           client;
    StaticBuffer*   name;
    int64_t         accountId;
    bool            isLocal;
} Main_ZZoneFromCharSelect;

typedef union {
    Main_ZZoneFromCharSelect    zZoneFromCharSelect;
} Main_ZPacket;

#endif/*MAIN_ZPACKET_H*/
