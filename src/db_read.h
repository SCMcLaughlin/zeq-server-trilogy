
#ifndef DB_READ_H
#define DB_READ_H

#include "define.h"
#include "db_thread.h"
#include "zpacket.h"
#include <sqlite3.h>

void dbr_dispatch(DbThread* db, sqlite3* sqlite, ZPacket* zpacket);
void dbr_destruct(ZPacket* zpacket, int zop);

#endif/*DB_READ_H*/
