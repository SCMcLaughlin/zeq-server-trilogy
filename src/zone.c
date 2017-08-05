
#include "zone.h"
#include "util_alloc.h"
#include "zone_thread.h"

struct Zone {
    int16_t     zoneId;
    int16_t     instId;
};

Zone* zone_create(int zoneId, int instId)
{
    Zone* zone = alloc_type(Zone);
    
    if (!zone) goto fail_alloc;
    
    zone->zoneId = (int16_t)zoneId;
    zone->instId = (int16_t)instId;
    
fail:
    zone_destroy(zone);
fail_alloc:
    return NULL;
}

Zone* zone_destroy(Zone* zone)
{
    if (zone)
    {
        free(zone);
    }
    
    return NULL;
}
