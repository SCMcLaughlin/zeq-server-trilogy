
#ifndef LOGIN_ZPACKET_H
#define LOGIN_ZPACKET_H

typedef struct {
    int8_t          initialStatus;
    int8_t          rank;
    int             serverId;
    StaticBuffer*   serverName;
    StaticBuffer*   remoteIpAddr;
    StaticBuffer*   localIpAddr;
} Login_ZNewServer;

typedef struct {
    int     serverId;
} Login_ZRemoveServer;

typedef struct {
    int     serverId;
    int     playerCount;
} Login_ZUpdatePlayerCount;

typedef struct {
    int     serverId;
    int8_t  status;
} Login_ZUpdateServerStatus;

typedef union {
    Login_ZNewServer            zNewServer;
    Login_ZRemoveServer         zRemoveServer;
    Login_ZUpdatePlayerCount    zUpdatePlayerCount;
    Login_ZUpdateServerStatus   zUpdateServerStatus;
} Login_ZPacket;

#endif/*LOGIN_ZPACKET_H*/
