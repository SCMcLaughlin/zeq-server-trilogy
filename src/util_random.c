
#include "util_random.h"
#include <sqlite3.h>

void random_bytes(void* buffer, int count)
{
    sqlite3_randomness(count, (byte*)buffer);
}

uint8_t random_uint8()
{
    uint8_t ret;
    random_bytes(&ret, sizeof(ret));
    return ret;
}
