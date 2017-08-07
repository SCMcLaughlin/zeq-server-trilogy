
#include "util_clock.h"

#define BASE_UNIX_TIME 921542400 /* 16 March 1999, 00:00:00 */
#define REAL_SECONDS_PER_MINUTE 3
#define DAYS_PER_YEAR 365 /* Norrath doesn't follow the Gregorian calendar, ok? */
#define MINUTES_PER_HOUR 60
#define MINUTES_PER_DAY (MINUTES_PER_HOUR * 24)
#define MINUTES_PER_YEAR (MINUTES_PER_DAY * DAYS_PER_YEAR)

void clock_calc_norrath_time_at(NorrathTime* out, uint64_t unixTime)
{
    uint64_t eq;
    uint32_t day;
    uint32_t i;
    
    static const uint8_t daysInMonth[12] = {
        31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
    };
    
    /* This gets us the EQ time in EQ minutes */
    eq = (unixTime - BASE_UNIX_TIME) / REAL_SECONDS_PER_MINUTE;
    
    /* Whittling our way down... get the EQ year and factor it out of our EQ minutes */
    out->year = (uint16_t)(eq / MINUTES_PER_YEAR);
    eq %= MINUTES_PER_YEAR;
    
    /* Months take some extra work */
    day = (uint32_t)((eq / MINUTES_PER_DAY) + 1); /* There's no zeroth day of the month */
    
    for (i = 0; i < 12; i++)
    {
        uint8_t inMonth = daysInMonth[i];
        
        if (day <= inMonth)
        {
            out->month = i + 1; /* No zeroth month of the year either */
            break;
        }
        
        day -= inMonth;
    }
    
    /* Day */
    out->day = day;
    eq %= MINUTES_PER_DAY;
    
    /* Hour */
    out->hour = (uint8_t)(eq / MINUTES_PER_HOUR);
    eq %= MINUTES_PER_HOUR;
    
    /* Minute */
    out->minute = (uint8_t)eq;
}

#undef REAL_SECONDS_PER_MINUTE
#undef DAYS_PER_YEAR
#undef MINUTES_PER_HOUR
#undef MINUTES_PER_DAY
#undef MINUTES_PER_YEAR
