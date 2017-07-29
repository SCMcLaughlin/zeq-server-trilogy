
#include "util_clock.h"
#include <time.h>

static LARGE_INTEGER sFrequency;
static BOOL sUseQpc = 0xffffffff;

static LARGE_INTEGER clock_win32()
{
    LARGE_INTEGER now;
    HANDLE thread;
    DWORD_PTR oldMask;

    if (sUseQpc == 0xffffffff)
    {
        sUseQpc = QueryPerformanceFrequency(&sFrequency);
    }
    
    thread = GetCurrentThread();
    oldMask = SetThreadAffinityMask(thread, 0);
    
    QueryPerformanceCounter(&now);
    SetThreadAffinityMask(thread, oldMask);
    
    return now;
}

uint64_t clock_milliseconds()
{
    LARGE_INTEGER li = clock_win32();
    return (uint64_t)((1000LL * li.QuadPart) / sFrequency.QuadPart);
}

uint64_t clock_microseconds()
{
    LARGE_INTEGER li = clock_win32();
    return (uint64_t)((1000000LL * li.QuadPart) / sFrequency.QuadPart);
}

uint64_t clock_unix_seconds()
{
    return _time64(NULL);
}

void clock_sleep(uint32_t ms)
{
    Sleep(ms);
}
