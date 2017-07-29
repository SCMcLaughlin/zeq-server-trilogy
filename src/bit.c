
#include "bit.h"

uint32_t bit_next_pow2_u32(uint32_t n)
{
    n--;
    
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    
    n++;
    
    return n;
}

uint32_t bit_pow2_greater_than_u32(uint32_t n)
{
    return bit_is_pow2(n) ? (n << 1) : bit_next_pow2_u32(n);
}

uint32_t bit_pow2_greater_or_equal_u32(uint32_t n)
{
    return bit_is_pow2(n) ? (n) : bit_next_pow2_u32(n);
}
