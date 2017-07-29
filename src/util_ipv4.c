
#include "util_ipv4.h"

#define make_ip(a, b, c, d) (((a << 0) & 0xff) | ((b << 8) & 0xff00) | ((c << 16) & 0xff0000) | ((d << 24) & 0xff000000))

bool ip_is_local(uint32_t ip)
{
    uint32_t check;

    /* Is it the loopback address 127.0.0.1? */
    check = make_ip(127, 0, 0, 1);
    if (ip == check)
        goto match;

    /* Is it on the 192.168.x.x subnet? */
    check = make_ip(192, 168, 0, 0);
    if ((ip & 0x0000ffff) == check)
        goto match;

    /* Is it on the 10.x.x.x subnet? */
    check = make_ip(10, 0, 0, 0);
    if ((ip & 0x000000ff) == check)
        goto match;

    /* Is it on the 172.16.0.0 to 172.31.255.255 subnet (12-bit mask)? */
    check = make_ip(172, 16, 0, 0);
    if ((ip & 0x0000f0ff) == check)
        goto match;

    return false;

match:
    return true;
}

#undef make_ip
