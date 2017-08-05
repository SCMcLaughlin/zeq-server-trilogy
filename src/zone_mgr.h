
#ifndef ZONE_MGR_H
#define ZONE_MGR_H

#include "define.h"
#include "buffer.h"
#include "client.h"

struct MainThread;

typedef struct {
    int16_t     zoneId;
    int16_t     instId;
    uint16_t    port;
} PortByZoneId;

typedef struct ZoneMgr {
    uint32_t        zoneCount;
    uint8_t         maxZoneThreads;
    uint16_t        nextZoneThreadPort;
    PortByZoneId*   portsByZoneId;
    StaticBuffer*   remoteIpAddress;
    StaticBuffer*   localIpAddress;
} ZoneMgr;

int zmgr_init(struct MainThread* mt);
void zmgr_deinit(ZoneMgr* zmgr);

int zmgr_add_client_from_char_select(struct MainThread* mt, Client* client, int zoneId, int instId, uint16_t* outPort);

StaticBuffer* zmgr_remote_ip(ZoneMgr* zmgr);
StaticBuffer* zmgr_local_ip(ZoneMgr* zmgr);

#endif/*ZONE_MGR_H*/
