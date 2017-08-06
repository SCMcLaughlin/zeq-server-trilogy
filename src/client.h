
#ifndef CLIENT_H
#define CLIENT_H

#include "define.h"
#include "buffer.h"
#include "client_load_data.h"
#include "define_netcode.h"

typedef struct Client Client;

Client* client_create_unloaded(StaticBuffer* name, int64_t accountId, IpAddr ipAddr, bool isLocal);
Client* client_destroy(Client* client);

void client_load_character_data(Client* client, ClientLoadData_Character* data);

StaticBuffer* client_name(Client* client);

bool client_is_local(Client* client);

#endif/*CLIENT_H*/
