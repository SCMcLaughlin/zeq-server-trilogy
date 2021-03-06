
#include "util_cap.h"

int8_t cap_int8(int v)
{
    return (v > INT8_MAX) ? INT8_MAX : ((v < INT8_MIN) ? INT8_MIN : v);
}

uint8_t cap_uint8(int v)
{
    return (v > UINT8_MAX) ? UINT8_MAX : ((v < 0) ? 0 : v);
}

int cap_min_max(int v, int vMin, int vMax)
{
    return (v > vMax) ? vMax : ((v < vMin) ? vMin : v);
}

int cap_max(int v, int vMax)
{
    return (v > vMax) ? vMax : v;
}
