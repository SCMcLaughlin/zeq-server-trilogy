
#ifndef DB_WRITE_H
#define DB_WRITE_H

#include "define.h"
#include "db_thread.h"
#include "zpacket.h"
#include <sqlite3.h>

void dbw_dispatch(DbThread* db, sqlite3* sqlite, ZPacket* zpacket);
void dbw_destruct(ZPacket* zpacket, int zop);

#endif/*DB_WRITE_H*/
