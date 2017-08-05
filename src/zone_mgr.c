
#include "zone_mgr.h"
#include "main_thread.h"

#define ZONE_MGR_FIRST_ZONE_THREAD_PORT 7000

static int zmgr_processor_thread_count()
{
#if defined(PLATFORM_WINDOWS)
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    return (int)info.dwNumberOfProcessors;
#elif defined(_SC_NPROCESSORS_ONLN)
    return sysconf(_SC_NPROCESSORS_ONLN);
#else
    return 0;
#endif
}

int zmgr_init(MainThread* mt)
{
    ZoneMgr* zmgr = mt_get_zmgr(mt);
    int threadsAvailable = zmgr_processor_thread_count();
    
    if (threadsAvailable <= 0)
        threadsAvailable = 1;

    zmgr->zoneCount = 0;
    zmgr->maxZoneThreads = (uint8_t)threadsAvailable;
    zmgr->nextZoneThreadPort = ZONE_MGR_FIRST_ZONE_THREAD_PORT;
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

int zmgr_add_client_from_char_select(MainThread* mt, Client* client, int zoneId, int instId, uint16_t* outPort)
{
    return ERR_None;
}

StaticBuffer* zmgr_remote_ip(ZoneMgr* zmgr)
{
    return zmgr->remoteIpAddress;
}

StaticBuffer* zmgr_local_ip(ZoneMgr* zmgr)
{
    return zmgr->localIpAddress;
}

