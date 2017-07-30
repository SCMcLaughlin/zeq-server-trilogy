
#include "util_socket_lib.h"

bool socket_lib_init()
{
#ifdef PLATFORM_WINDOWS
    WORD wsaVer = MAKEWORD(2, 2);
    WSADATA wsaData;
    int err;
    
    err = WSAStartup(wsaVer, &wsaData);
    
    if (err)
    {
        fprintf(stderr, "WSAStartup() failed, errcode: %i\n", err);
        return false;
    }
    
    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
    {
        fprintf(stderr, "WSAStartup() could not open version 2.2, got %i.%i instead\n", LOBYTE(wsaData.wVersion), HIBYTE(wsaData.wVersion));
        return false;
    }
    
#endif
    return true;
}

void socket_lib_deinit()
{
#ifdef PLATFORM_WINDOWS
    WSACleanup();
#endif
}
