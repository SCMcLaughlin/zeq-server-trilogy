
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

#pragma pack()

#endif/*LOGIN_PACKET_STRUCT_H*/
