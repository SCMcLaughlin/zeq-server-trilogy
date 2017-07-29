
#ifndef UTIL_ATOMIC_H
#define UTIL_ATOMIC_H

#include "define.h"

#if defined(PLATFORM_LINUX)
# include <stdatomic.h>
#elif defined(PLATFORM_WINDOWS)
# include <atomic>
#endif

#ifdef PLATFORM_WINDOWS
typedef std::atomic_int32_t atomic32_t;
typedef std::atomic_int16_t atomic16_t;
typedef std::atomic_int8_t atomic8_t;
#else
typedef atomic_int atomic32_t;
typedef atomic_short atomic16_t;
typedef atomic_char atomic8_t;
#endif

void atomic32_set(atomic32_t* a, int32_t val);
int32_t atomic32_get(atomic32_t* a);
int32_t atomic32_add(atomic32_t* a, int32_t amt);
int32_t atomic32_sub(atomic32_t* a, int32_t amt);
int atomic32_cmp_xchg_weak(atomic32_t* a, int32_t expected, int32_t desired);
int atomic32_cmp_xchg_strong(atomic32_t* a, int32_t expected, int32_t desired);

void atomic16_set(atomic16_t* a, int16_t val);
int16_t atomic16_get(atomic16_t* a);
int atomic16_cmp_xchg_weak(atomic16_t* a, int16_t expected, int16_t desired);
int atomic16_cmp_xchg_strong(atomic16_t* a, int16_t expected, int16_t desired);

void atomic8_set(atomic8_t* a, int8_t val);
int8_t atomic8_get(atomic8_t* a);
int atomic8_cmp_xchg_weak(atomic8_t* a, int8_t expected, int8_t desired);
int atomic8_cmp_xchg_strong(atomic8_t* a, int8_t expected, int8_t desired);

#endif/*UTIL_ATOMIC_H*/
