
#include "util_atomic.h"

void atomic32_set(atomic32_t* a, int32_t val)
{
    a->store(val);
}

int32_t atomic32_get(atomic32_t* a)
{
    return a->load();
}

int32_t atomic32_add(atomic32_t* a, int32_t amt)
{
    return a->fetch_add(amt);
}

int32_t atomic32_sub(atomic32_t* a, int32_t amt)
{
    return a->fetch_sub(amt);
}

int atomic32_cmp_xchg_weak(atomic32_t* a, int32_t expected, int32_t desired)
{
    return a->compare_exchange_weak(expected, desired);
}

int atomic32_cmp_xchg_strong(atomic32_t* a, int32_t expected, int32_t desired)
{
    return a->compare_exchange_strong(expected, desired);
}

void atomic16_set(atomic16_t* a, int16_t val)
{
    a->store(val);
}

int16_t atomic16_get(atomic16_t* a)
{
    return a->load();
}

int atomic16_cmp_xchg_weak(atomic16_t* a, int16_t expected, int16_t desired)
{
    return a->compare_exchange_weak(expected, desired);
}

int atomic16_cmp_xchg_strong(atomic16_t* a, int16_t expected, int16_t desired)
{
    return a->compare_exchange_strong(expected, desired);
}

void atomic8_set(atomic8_t* a, int8_t val)
{
    a->store(val);
}

int8_t atomic8_get(atomic8_t* a)
{
    return a->load();
}

int atomic8_cmp_xchg_weak(atomic8_t* a, int8_t expected, int8_t desired)
{
    return a->compare_exchange_weak(expected, desired);
}

int atomic8_cmp_xchg_strong(atomic8_t* a, int8_t expected, int8_t desired)
{
    return a->compare_exchange_strong(expected, desired);
}
