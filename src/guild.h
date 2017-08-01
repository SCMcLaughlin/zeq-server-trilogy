
#ifndef GUILD_H
#define GUILD_H

#include "define.h"
#include "buffer.h"

typedef struct {
    uint32_t        guildId;
    StaticBuffer*   guildName;
} Guild;

#endif/*GUILD_H*/
