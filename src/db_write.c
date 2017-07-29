
#include "db_write.h"
#include "enum_zop.h"

void dbw_dispatch(DbThread* db, sqlite3* sqlite, ZPacket* zpacket)
{
    switch (zpacket->db.zQuery.zop)
    {
    case ZOP_DB_QueryLoginNewAccount:
        break;
    
    default:
        break;
    }
}

void dbw_destruct(ZPacket* zpacket, int zop)
{
    switch (zop)
    {
    default:
        break;
    }
}
