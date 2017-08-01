
#ifndef CHAR_SELECT_ZPACKET_H
#define CHAR_SELECT_ZPACKET_H

#include "define.h"
#include "guild.h"

typedef struct {
    bool    isLast;
    Guild   guild;
} CharSelect_ZAddGuild;

typedef struct {
    int64_t     accountId;
    char        sessionKey[16];
} CharSelect_ZLoginAuth;

typedef union {
    CharSelect_ZAddGuild    zAddGuild;
    CharSelect_ZLoginAuth   zLoginAuth;
} CharSelect_ZPacket;

#endif/*CHAR_SELECT_ZPACKET_H*/
