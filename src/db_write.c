
#include "db_write.h"
#include "util_clock.h"
#include "enum_zop.h"
#include "enum2str.h"

void dbw_dispatch(DbThread* db, sqlite3* sqlite, ZPacket* zpacket)
{
    uint64_t timer = clock_microseconds();
    int zop = zpacket->db.zQuery.zop;
    
    switch (zop)
    {
    case ZOP_DB_QueryLoginNewAccount:
        break;
    
    default:
        log_writef(db_get_log_queue(db), db_get_log_id(db), "WARNING: dbw_dispatch: received unexpected zop: %s", enum2str_zop(zop));
        goto destruct;
    }
    
    timer = clock_microseconds() - timer;
    log_writef(db_get_log_queue(db), db_get_log_id(db), "Executed WRITE query %s in %llu microseconds", enum2str_zop(zop), timer);
destruct:
    dbw_destruct(db, zpacket, zop);
}

void dbw_destruct(DbThread* db, ZPacket* zpacket, int zop)
{
    switch (zop)
    {
    default:
        log_writef(db_get_log_queue(db), db_get_log_id(db), "WARNING: dbw_destruct: received unexpected zop: %s", enum2str_zop(zop));
        break;
    }
}
