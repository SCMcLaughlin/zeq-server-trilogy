
#ifndef ZONE_MGR_H
#define ZONE_MGR_H

#include "define.h"
#include "buffer.h"

struct MainThread;

typedef struct {
    int16_t     zoneId;
    int16_t     instId;
    uint16_t    port;
} PortByZoneId;

typedef struct ZoneMgr {
    uint32_t        zoneCount;
    PortByZoneId*   portsByZoneId;
    StaticBuffer*   remoteIpAddress;
    StaticBuffer*   localIpAddress;
} ZoneMgr;

int zmgr_init(struct MainThread* mt);
void zmgr_deinit(ZoneMgr* zmgr);

int zmgr_start_zone_if_not_up(struct MainThread* mt, int zoneId, int instId);

StaticBuffer* zmgr_remote_ip(ZoneMgr* zmgr);
StaticBuffer* zmgr_local_ip(ZoneMgr* zmgr);

#endif/*ZONE_MGR_H*/
