
#include "util_atomic.h"

void atomic32_set(atomic32_t* a, int32_t val)
{
    atomic_store(a, val);
}

int32_t atomic32_get(atomic32_t* a)
{
    return atomic_load(a);
}

int32_t atomic32_add(atomic32_t* a, int32_t amt)
{
    return atomic_fetch_add(a, amt);
}

int32_t atomic32_sub(atomic32_t* a, int32_t amt)
{
    return atomic_fetch_sub(a, amt);
}

int atomic32_cmp_xchg_weak(atomic32_t* a, int32_t expected, int32_t desired)
{
    return atomic_compare_exchange_weak(a, &expected, desired);
}

int atomic32_cmp_xchg_strong(atomic32_t* a, int32_t expected, int32_t desired)
{
    return atomic_compare_exchange_strong(a, &expected, desired);
}

void atomic16_set(atomic16_t* a, int16_t val)
{
    atomic_store(a, val);
}

int16_t atomic16_get(atomic16_t* a)
{
    return atomic_load(a);
}

int atomic16_cmp_xchg_weak(atomic16_t* a, int16_t expected, int16_t desired)
{
    return atomic_compare_exchange_weak(a, &expected, desired);
}

int atomic16_cmp_xchg_strong(atomic16_t* a, int16_t expected, int16_t desired)
{
    return atomic_compare_exchange_strong(a, &expected, desired);
}

void atomic8_set(atomic8_t* a, int8_t val)
{
    atomic_store(a, val);
}

int8_t atomic8_get(atomic8_t* a)
{
    return atomic_load(a);
}

int atomic8_cmp_xchg_weak(atomic8_t* a, int8_t expected, int8_t desired)
{
    return atomic_compare_exchange_weak(a, (char*)&expected, desired);
}

int atomic8_cmp_xchg_strong(atomic8_t* a, int8_t expected, int8_t desired)
{
    return atomic_compare_exchange_strong(a, (char*)&expected, desired);
}
