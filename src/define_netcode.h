
#ifndef DEFINE_NETCODE_H
#define DEFINE_NETCODE_H

#include "define.h"

#ifndef PLATFORM_WINDOWS
# include <errno.h>
# include <unistd.h>
# include <fcntl.h>
# include <sys/types.h>
# include <sys/socket.h>
# include <sys/time.h>
# include <netdb.h>
# include <signal.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <netinet/tcp.h>
# include <net/if.h>
# include <endian.h>
#endif

#define to_network_uint16 htons
#define to_network_uint32 htonl
#define to_host_uint16 ntohs
#define to_host_uint32 ntohl

#ifdef PLATFORM_WINDOWS
# define to_network_uint64 htonll
# define to_host_uint64 ntohll
#else
# define to_network_uint64 htobe64
# define to_host_uint64 be64toh
#endif

#ifdef PLATFORM_WINDOWS
typedef SOCKET sock_t;
typedef int socklen_t;
#else
typedef int sock_t;
# define closesocket close
# define INVALID_SOCKET -1
#endif

typedef struct {
    uint32_t    ip;
    uint16_t    port;
} IpAddr;

typedef struct sockaddr_in IpAddrRaw;

#endif/*DEFINE_NETCODE_H*/
