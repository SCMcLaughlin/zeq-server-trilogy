
#ifndef CLIENT_MGR_H
#define CLIENT_MGR_H

#include "define.h"
#include "client.h"
#include "client_load_data.h"
#include "guild.h"
#include "util_hash_tbl.h"
#include "zpacket.h"

struct MainThread;

typedef struct {
    Client*     client;
    void*       csClient;
    int         queriesCompleted;
    int16_t     zoneId;
    int16_t     instId;
} ClientLoading;

typedef struct ClientMgr {
    uint32_t        clientCount;
    uint32_t        loadingClientCount;
    uint32_t        guildCount;
    Client**        clients;
    ClientLoading*  loadingClients;
    HashTbl         clientsByName;
    Guild*          guilds;
} ClientMgr;

int cmgr_init(struct MainThread* mt);
void cmgr_deinit(ClientMgr* cmgr);

void cmgr_handle_guild_list(struct MainThread* mt, ZPacket* zpacket);
void cmgr_handle_zone_from_char_select(struct MainThread* mt, ZPacket* zpacket);
void cmgr_handle_load_character(struct MainThread* mt, ZPacket* zpacket);

#endif/*CLIENT_MGR_H*/
