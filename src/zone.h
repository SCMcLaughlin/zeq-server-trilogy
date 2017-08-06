
#ifndef ZONE_H
#define ZONE_H

#include "define.h"
#include "log_thread.h"

typedef struct Zone Zone;

Zone* zone_create(LogThread* log, int zoneId, int instId);
Zone* zone_destroy(Zone* zone);

#endif/*ZONE_H*/
