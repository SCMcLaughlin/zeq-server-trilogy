
#include "client.h"
#include "mob.h"
#include "util_alloc.h"

struct Client {
    Mob             mob;
    StaticBuffer*   surname;
    int64_t         clientId;
    int64_t         accountId;
    bool            isLocal;
};

Client* client_create_unloaded(StaticBuffer* name, int64_t accountId, bool isLocal)
{
    Client* client = alloc_type(Client);

    if (!client) return NULL;

    memset(client, 0, sizeof(Client));

    client->mob.parentType = MOB_PARENT_TYPE_Client;
    mob_set_name_sbuf(&client->mob, name);
    sbuf_grab(name);

    client->accountId = accountId;
    client->isLocal = isLocal;

    return client;
}

Client* client_destroy(Client* client)
{
    if (client)
    {
        mob_deinit(&client->mob);
        client->surname = sbuf_drop(client->surname);
    }

    return NULL;
}

void client_load_character_data(Client* client, ClientLoadData_Character* data)
{
    client->surname = data->surname;
    client->mob.level = data->level;
    client->mob.classId = data->classId;
    client->mob.genderId = data->genderId;
    client->mob.faceId = data->faceId;
    client->mob.raceId = data->raceId;
    client->mob.deityId = data->deityId;
    client->mob.loc = data->loc;
    mob_set_cur_hp_no_cap_check(&client->mob, data->currentHp);
    mob_set_cur_mana_no_cap_check(&client->mob, data->currentMana);
    mob_set_cur_endurance_no_cap_check(&client->mob, data->currentEndurance);
    client->mob.baseStats.STR = data->STR;
    client->mob.baseStats.STA = data->STA;
    client->mob.baseStats.DEX = data->DEX;
    client->mob.baseStats.AGI = data->AGI;
    client->mob.baseStats.INT = data->INT;
    client->mob.baseStats.WIS = data->WIS;
    client->mob.baseStats.CHA = data->CHA;

    /*fixme: calc base resists and max hp & mana based on level, class, race*/

    /*fixme: finish this*/
    /*
    int64_t         experience;
    uint16_t        trainingPoints;

    uint8_t         guildRank;
    uint32_t        guildId;
    uint64_t        harmtouchTimestamp;
    uint64_t        disciplineTimestamp;
    Coin            coin;
    Coin            coinCursor;
    Coin            coinBank;
    uint16_t        hunger;
    uint16_t        thirst;
    bool            isGM;
    uint8_t         anon;
    uint16_t        drunkeness;
    uint64_t        creationTimestamp;
    */
}

StaticBuffer* client_name_sbuf(Client* client)
{
    return client->mob.name;
}
