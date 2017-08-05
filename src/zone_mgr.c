
#include "zone_mgr.h"
#include "main_thread.h"

int zmgr_init(MainThread* mt)
{
    ZoneMgr* zmgr = mt_get_zmgr(mt);

    zmgr->zoneCount = 0;
    zmgr->portsByZoneId = NULL;
    zmgr->remoteIpAddress = NULL;
    zmgr->localIpAddress = NULL;

    /*fixme: read these from a script/config file*/
    zmgr->remoteIpAddress = sbuf_create_from_literal("127.0.0.1");
    if (!zmgr->remoteIpAddress) goto oom;
    sbuf_grab(zmgr->remoteIpAddress);

    zmgr->localIpAddress = sbuf_create_from_literal("127.0.0.1");
    if (!zmgr->localIpAddress) goto oom;
    sbuf_grab(zmgr->localIpAddress);

    return ERR_None;
oom:
    return ERR_OutOfMemory;
}

void zmgr_deinit(ZoneMgr* zmgr)
{
    free_if_exists(zmgr->portsByZoneId);
    zmgr->remoteIpAddress = sbuf_drop(zmgr->remoteIpAddress);
    zmgr->localIpAddress = sbuf_drop(zmgr->localIpAddress);
}

StaticBuffer* zmgr_remote_ip(ZoneMgr* zmgr)
{
    return zmgr->remoteIpAddress;
}

StaticBuffer* zmgr_local_ip(ZoneMgr* zmgr)
{
    return zmgr->localIpAddress;
}

