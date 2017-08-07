
#include "client.h"
#include "define_netcode.h"
#include "inventory.h"
#include "loc.h"
#include "misc_enum.h"
#include "misc_struct.h"
#include "mob.h"
#include "skills.h"
#include "spellbook.h"
#include "util_alloc.h"

struct Client {
    Mob             mob;
    int             zoneIndex;
    StaticBuffer*   surname;
    int64_t         characterId;
    int64_t         accountId;
    IpAddr          ipAddr;
    uint16_t        isLocal             : 1;
    uint16_t        isAutoSplitEnabled  : 1;
    uint16_t        isPvP               : 1;
    uint16_t        isGM                : 1;
    uint16_t        isAfk               : 1;
    uint16_t        isLinkdead          : 1;
    uint16_t        isSneaking          : 1;
    uint16_t        isHiding            : 1;
    uint16_t        isGMSpeed           : 1;
    uint16_t        isGMHide            : 1;
    int64_t         experience;
    uint64_t        harmtouchTimestamp;
    uint64_t        disciplineTimestamp;
    uint64_t        creationTimestamp;
    uint16_t        trainingPoints;
    uint16_t        hunger;
    uint16_t        thirst;
    uint16_t        drunkeness;
    uint32_t        guildId;
    Coin            coin;
    Coin            coinCursor;
    Coin            coinBank;
    uint8_t         anon;
    uint8_t         guildRank;
    Inventory       inventory;
    Skills          skills;
    Spellbook       spellbook;
    BindPoint       bindPoint[5];
};

Client* client_create_unloaded(StaticBuffer* name, int64_t accountId, IpAddr ipAddr, bool isLocal)
{
    Client* client = alloc_type(Client);

    if (!client) return NULL;

    memset(client, 0, sizeof(Client));

    mob_init_client_unloaded(&client->mob, name);

    client->zoneIndex = -1;
    client->surname = NULL;
    client->characterId = -1;
    client->accountId = accountId;
    client->ipAddr = ipAddr;
    client->isLocal = isLocal;

    return client;
}

Client* client_destroy(Client* client)
{
    if (client)
    {
        mob_deinit(&client->mob);
        inv_deinit(&client->inventory);
        spellbook_deinit(&client->spellbook);
        client->surname = sbuf_drop(client->surname);
    }

    return NULL;
}

void client_load_character_data(Client* client, ClientLoadData_Character* data)
{
    mob_init_client_character(&client->mob, data);
    
    client->characterId = data->charId;
    client->surname = data->surname;
    
    client->isAutoSplitEnabled = data->autoSplit;
    client->isPvP = data->isPvP;
    client->isGM = data->isGM;
    client->isAfk = false;
    client->isLinkdead = false;
    client->isSneaking = false;
    client->isHiding = false;
    client->isGMSpeed = data->isGMSpeed;
    client->isGMHide = data->isGMHide;
    
    client->experience = data->experience;
    client->harmtouchTimestamp = data->harmtouchTimestamp;
    client->disciplineTimestamp = data->disciplineTimestamp;
    client->creationTimestamp = data->creationTimestamp;
    client->trainingPoints = data->trainingPoints;
    client->hunger = data->hunger;
    client->thirst = data->thirst;
    client->drunkeness = data->drunkeness;
    client->guildId = data->guildId;
    client->coin = data->coin;
    client->coinCursor = data->coinCursor;
    client->coinBank = data->coinBank;
    client->isGM = data->isGM;
    client->anon = data->anon;
    client->guildRank = data->guildRank;
    
    inv_init(&client->inventory);
    skills_init(&client->skills, client->mob.classId, client->mob.baseRaceId);

    /*fixme: calc base resists and max hp & mana based on level, class, race*/
}

Mob* client_mob(Client* client)
{
    return &client->mob;
}

StaticBuffer* client_name(Client* client)
{
    return client->mob.name;
}

const char* client_name_str(Client* client)
{
    return sbuf_str(client->mob.name);
}

const char* client_surname_str_no_null(Client* client)
{
    StaticBuffer* surname = client->surname;
    return (surname) ? sbuf_str(surname) : "";
}

uint8_t client_base_gender_id(Client* client)
{
    return client->mob.baseGenderId;
}

uint16_t client_base_race_id(Client* client)
{
    return client->mob.baseRaceId;
}

uint8_t client_class_id(Client* client)
{
    return client->mob.classId;
}

uint16_t client_deity_id(Client* client)
{
    return client->mob.deityId;
}

uint8_t client_level(Client* client)
{
    return client->mob.level;
}

int64_t client_experience(Client* client)
{
    return client->experience;
}

uint16_t client_training_points(Client* client)
{
    return client->trainingPoints;
}

int64_t client_cur_mana(Client* client)
{
    return client->mob.currentMana;
}

uint8_t client_face_id(Client* client)
{
    return client->mob.faceId;
}

int64_t client_cur_hp(Client* client)
{
    return client->mob.currentHp;
}

uint8_t client_base_str(Client* client)
{
    return client->mob.baseStats.STR;
}

uint8_t client_base_sta(Client* client)
{
    return client->mob.baseStats.STA;
}

uint8_t client_base_cha(Client* client)
{
    return client->mob.baseStats.CHA;
}

uint8_t client_base_dex(Client* client)
{
    return client->mob.baseStats.DEX;
}

uint8_t client_base_int(Client* client)
{
    return client->mob.baseStats.INT;
}

uint8_t client_base_agi(Client* client)
{
    return client->mob.baseStats.AGI;
}

uint8_t client_base_wis(Client* client)
{
    return client->mob.baseStats.WIS;
}

float client_loc_x(Client* client)
{
    return client->mob.loc.x;
}

float client_loc_y(Client* client)
{
    return client->mob.loc.y;
}

float client_loc_z(Client* client)
{
    return client->mob.loc.z;
}

float client_loc_heading(Client* client)
{
    return client->mob.loc.heading;
}

Coin* client_coin(Client* client)
{
    return &client->coin;
}

Coin* client_coin_bank(Client* client)
{
    return &client->coinBank;
}

Coin* client_coin_cursor(Client* client)
{
    return &client->coinCursor;
}

uint32_t client_guild_id(Client* client)
{
    return client->guildId;
}

uint32_t client_guild_id_or_ffff(Client* client)
{
    uint32_t id = client->guildId;
    return (id) ? id : 0xffff;
}

uint8_t client_guild_rank(Client* client)
{
    return client->guildRank;
}

uint8_t client_guild_rank_or_ff(Client* client)
{
    uint8_t rank = client->guildRank;
    return (rank) ? rank : 0xff;
}

uint8_t client_anon_setting(Client* client)
{
    return client->anon;
}

Inventory* client_inv(Client* client)
{
    return &client->inventory;
}

Skills* client_skills(Client* client)
{
    return &client->skills;
}

Spellbook* client_spellbook(Client* client)
{
    return &client->spellbook;
}

Zone* client_get_zone(Client* client)
{
    return mob_get_zone(&client->mob);
}

void client_reset_for_zone(Client* client, Zone* zone)
{
    Mob* mob = &client->mob;
    
    mob_set_zone(mob, zone);
    mob_set_entity_id(mob, -1);
    mob_set_zone_index(mob, -1);
    mob_set_upright_state(mob, UPRIGHT_STATE_Standing);
    client_set_zone_index(client, -1);
}

bool client_is_local(Client* client)
{
    return client->isLocal;
}

IpAddr client_ip_addr(Client* client)
{
    return client->ipAddr;
}

void client_set_ip_addr(Client* client, IpAddr ipAddr)
{
    client->ipAddr = ipAddr;
}

uint32_t client_ip(Client* client)
{
    return client->ipAddr.ip;
}

uint16_t client_port(Client* client)
{
    return client->ipAddr.port;
}

void client_set_port(Client* client, uint16_t port)
{
    client->ipAddr.port = port;
}

bool client_is_auto_split_enabled(Client* client)
{
    return client->isAutoSplitEnabled != 0;
}

bool client_is_pvp(Client* client)
{
    return client->isPvP != 0;
}

bool client_is_gm(Client* client)
{
    return client->isGM != 0;
}

bool client_is_afk(Client* client)
{
    return client->isAfk != 0;
}

bool client_is_linkdead(Client* client)
{
    return client->isLinkdead != 0;
}

bool client_is_sneaking(Client* client)
{
    return client->isSneaking != 0;
}

bool client_is_hiding(Client* client)
{
    return client->isHiding != 0;
}

bool client_has_gm_speed(Client* client)
{
    return client->isGMSpeed != 0;
}

bool client_has_gm_hide(Client* client)
{
    return client->isGMHide != 0;
}

uint64_t client_disc_timestamp(Client* client)
{
    return client->disciplineTimestamp;
}

uint64_t client_harmtouch_timestamp(Client* client)
{
    return client->harmtouchTimestamp;
}

uint64_t client_creation_timestamp(Client* client)
{
    return client->creationTimestamp;
}

uint16_t client_hunger(Client* client)
{
    return client->hunger;
}

uint16_t client_thirst(Client* client)
{
    return client->thirst;
}

uint16_t client_drunkeness(Client* client)
{
    return client->drunkeness;
}

uint8_t client_helm_texture_id(Client* client)
{
    return client->mob.helmTextureId;
}

uint8_t client_texture_id(Client* client)
{
    return client->mob.textureId;
}

uint8_t client_primary_weapon_model_id(Client* client)
{
    return client->mob.primaryWeaponModelId;
}

uint8_t client_secondary_weapon_model_id(Client* client)
{
    return client->mob.secondaryWeaponModelId;
}

uint8_t client_upright_state(Client* client)
{
    return client->mob.uprightState;
}

uint8_t client_light_level(Client* client)
{
    return client->mob.lightLevel;
}

uint8_t client_body_type(Client* client)
{
    return client->mob.bodyType;
}

float client_base_walking_speed(Client* client)
{
    return client->mob.baseWalkingSpeed;
}

float client_base_running_speed(Client* client)
{
    return client->mob.baseRunningSpeed;
}

float client_walking_speed(Client* client)
{
    return client->mob.currentWalkingSpeed;
}

float client_running_speed(Client* client)
{
    return client->mob.currentRunningSpeed;
}

BindPoint* client_bind_point(Client* client, int n)
{
    return &client->bindPoint[n];
}

int16_t client_entity_id(Client* client)
{
    return mob_entity_id(&client->mob);
}

int client_zone_index(Client* client)
{
    return client->zoneIndex;
}

void client_set_zone_index(Client* client, int index)
{
    client->zoneIndex = index;
}

int client_mob_zone_index(Client* client)
{
    return mob_zone_index(&client->mob);
}

void client_set_mob_zone_index(Client* client, int index)
{
    mob_set_zone_index(&client->mob, index);
}
