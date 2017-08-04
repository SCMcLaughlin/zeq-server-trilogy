
#include "hash.h"
#include "bit.h"

static uint32_t hash_int64_impl(uint32_t lo, uint32_t hi)
{
    lo ^= hi;
    hi = bit_rotate(hi, 14);
    lo -= hi;
    hi = bit_rotate(hi, 5);
    hi ^= lo;
    hi -= bit_rotate(lo, 13);
    return hi;
}

uint32_t hash_int64(int64_t key)
{
    key++;
    return hash_int64_impl((uint32_t)(key & 0x00000000ffffffLL), (uint32_t)((key & 0xffffffff00000000LL) >> 32LL));
}

uint32_t hash_str(const char* key, uint32_t len)
{
    uint32_t h = len;
    uint32_t step = (len >> 5) + 1;
    uint32_t i;

    for (i = len; i >= step; i -= step)
    {
        h = h ^ ((h << 5) + (h >> 2) + (key[i - 1]));
    }

    return h;
}
