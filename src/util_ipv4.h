
#ifndef UTIL_IPV4_H
#define UTIL_IPV4_H

#include "define.h"
#include "define_netcode.h"

/* ip must be in network byte order */
bool ip_is_local(uint32_t ip);

#endif/*UTIL_IPV4_H*/
