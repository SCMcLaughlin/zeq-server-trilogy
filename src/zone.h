
#ifndef ZONE_H
#define ZONE_H

#include "define.h"
#include "log_thread.h"

typedef struct Zone Zone;

struct Client;

Zone* zone_create(LogThread* log, int zoneId, int instId);
Zone* zone_destroy(Zone* zone);

void zone_add_client(Zone* zone, struct Client* client);

RingBuf* zone_log_queue(Zone* zone);
int zone_log_id(Zone* zone);

#endif/*ZONE_H*/
