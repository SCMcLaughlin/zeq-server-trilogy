
#include "util_random.h"
#include <sqlite3.h>

void random_bytes(void* buffer, int count)
{
    sqlite3_randomness(count, (byte*)buffer);
}
