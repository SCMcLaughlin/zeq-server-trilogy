
#ifndef UTIL_CLOCK_H
#define UTIL_CLOCK_H

#include "define.h"

typedef struct {
    uint8_t     hour;
    uint8_t     minute;
    uint8_t     day;
    uint8_t     month;
    uint16_t    year;
} NorrathTime;

uint64_t clock_milliseconds();
uint64_t clock_microseconds();
uint64_t clock_unix_seconds();
void clock_sleep(uint32_t ms);
void clock_calc_norrath_time_at(NorrathTime* out, uint64_t unixTime);
#define clock_calc_norrath_time(out) clock_calc_norrath_time_at((out), clock_unix_seconds())

#endif/*UTIL_CLOCK_H*/
