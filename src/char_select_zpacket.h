
#ifndef CHAR_SELECT_ZPACKET_H
#define CHAR_SELECT_ZPACKET_H

#include "define.h"

typedef struct {
    int64_t     accountId;
    char        sessionKey[16];
} CharSelect_ZLoginAuth;

typedef union {
    CharSelect_ZLoginAuth   zLoginAuth;
} CharSelect_ZPacket;

#endif/*CHAR_SELECT_ZPACKET_H*/
