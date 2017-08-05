
#include "mob.h"

void mob_deinit(Mob* mob)
{
    mob->name = sbuf_drop(mob->name);
}
