
#ifndef LOGIN_PACKET_STRUCT_H
#define LOGIN_PACKET_STRUCT_H

#include "define.h"

#pragma pack(1)

typedef struct {
    char    username[20];
    char    password[20];
} PSLogin_Credentials;

typedef struct {
    char        sessionId[10];
    char        unused[7];
    uint32_t    unknown;
} PSLogin_Session;

typedef struct {
    uint8_t     serverCount;
    uint8_t     unknown[2];
    uint8_t     showNumPlayers; /* 0xff to show the count, 0 to show "UP" */
} PSLogin_ServerListHeader;

typedef struct {
    uint8_t     isGreenName;
    int         playerCount;    /* -1 for "DOWN", -2 for "LOCKED" */
} PSLogin_ServerFooter;

typedef struct {
    uint32_t    admin;
    uint8_t     zeroesA[8];
    uint8_t     kunark;
    uint8_t     velious;
    uint8_t     zeroesB[12];
} PSLogin_ServerListFooter;

typedef struct {
    uint8_t     unknown;
    char        sessionKey[16];
} PSLogin_SessionKey;

#pragma pack()

#endif/*LOGIN_PACKET_STRUCT_H*/
