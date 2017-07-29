
#ifndef UTIL_CLOCK_H
#define UTIL_CLOCK_H

#include "define.h"

uint64_t clock_milliseconds();
uint64_t clock_microseconds();
uint64_t clock_unix_seconds();
void clock_sleep(uint32_t ms);

#endif/*UTIL_CLOCK_H*/
