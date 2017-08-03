
#ifndef CLIENT_MGR_H
#define CLIENT_MGR_H

#include "define.h"
#include "guild.h"
#include "zpacket.h"

struct MainThread;

typedef struct ClientMgr {
    uint32_t    guildCount;
    Guild*      guilds;
} ClientMgr;

int cmgr_init(struct MainThread* mt);
void cmgr_deinit(ClientMgr* cmgr);

void cmgr_handle_guild_list(struct MainThread* mt, ZPacket* zpacket);

#endif/*CLIENT_MGR_H*/
