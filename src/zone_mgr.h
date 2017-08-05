
#ifndef ZONE_MGR_H
#define ZONE_MGR_H

#include "define.h"
#include "buffer.h"
#include "client.h"
#include "ringbuf.h"
#include "zone_thread.h"

struct MainThread;

typedef struct {
    int16_t     zoneId;
    int16_t     instId;
    uint8_t     ztIndex;
} ZoneThreadByZoneId;

typedef struct {
    ZoneThread* zt;
    RingBuf*    ztQueue;
    uint16_t    port;
    uint16_t    zoneCount;
    uint32_t    clientCount;
} ZoneThreadWithQueue;

typedef struct ZoneMgr {
    uint32_t                zoneCount;
    uint8_t                 maxZoneThreads;
    uint8_t                 zoneThreadCount;
    uint16_t                nextZoneThreadPort;
    ZoneThreadWithQueue*    zoneThreads;
    ZoneThreadByZoneId*     ztByZoneId;
    StaticBuffer*           remoteIpAddress;
    StaticBuffer*           localIpAddress;
} ZoneMgr;

int zmgr_init(struct MainThread* mt);
void zmgr_deinit(ZoneMgr* zmgr);

int zmgr_add_client_from_char_select(struct MainThread* mt, Client* client, int zoneId, int instId, uint16_t* outPort);

StaticBuffer* zmgr_remote_ip(ZoneMgr* zmgr);
StaticBuffer* zmgr_local_ip(ZoneMgr* zmgr);

#endif/*ZONE_MGR_H*/
