
#include "client.h"
#include "class_id.h"
#include "client_packet_send.h"
#include "define_netcode.h"
#include "inventory.h"
#include "loc.h"
#include "misc_enum.h"
#include "misc_struct.h"
#include "mob.h"
#include "packet_create.h"
#include "skills.h"
#include "spellbook.h"
#include "util_alloc.h"
#include "util_cap.h"

struct Client {
    Mob             mob;
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
    uint32_t        weight;
    uint32_t        guildId;
    uint32_t        zoneInCount;
    int             hpFromItems;
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

    client->surname = NULL;
    client->characterId = -1;
    client->accountId = accountId;
    client->ipAddr = ipAddr;
    client->isLocal = isLocal;

    return client;
}

Client* client_destroy_no_zone(Client* client)
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

Client* client_destroy(Client* client)
{
    if (client)
    {
        zlua_deinit_client(client);
        return client_destroy_no_zone(client);
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
}

void client_calc_stats_all(Client* client)
{
    uint8_t level = client_level(client);
    uint8_t classId = client_class_id(client);
    uint16_t raceId = client_base_race_id(client);
    Mob* mob = client_mob(client);
    CoreStats* total = mob_total_stats(mob);
    CoreStats* base = mob_base_stats(mob);
    CoreStats* capped = mob_capped_stats(mob);
    int defense = client_get_skill(client, SKILL_Defense);
    
    /* Step 0: reset totals to 0 */
    memset(total, 0, sizeof(CoreStats));
    mob->acFromAGI = 0;
    mob->acFromBonus = 0;
    mob->acFromSkill = 0;
    mob->acFromItems = 0;
    mob->acFromBuffs = 0;
    mob->ac = 0;
    
    /* Step 1: calc total stats from inventory items */
    inv_calc_stats(client_inv(client), total, &client->weight, &mob->acFromItems);
    /* The client adds HP from items on its end, so we need to be able to subtract them when we do HP updates */
    client->hpFromItems = total->maxHp;
    
    /* Step 2: calc total stats from buffs */
    
    /* Step 3: calc base resists */
    
    /* Step 4: add base stats and resists to totals */
    total->STR += base->STR;
    total->STA += base->STA;
    total->DEX += base->DEX;
    total->AGI += base->AGI;
    total->INT += base->INT;
    total->WIS += base->WIS;
    total->CHA += base->CHA;
    total->svMagic += base->svMagic;
    total->svFire += base->svFire;
    total->svCold += base->svCold;
    total->svPoison += base->svPoison;
    total->svDisease += base->svDisease;
    
    /* Step 5: cap base stats and resists */
    capped->STR = cap_min_max(total->STR, -255, 255);
    capped->STA = cap_min_max(total->STA, -255, 255);
    capped->DEX = cap_min_max(total->DEX, -255, 255);
    capped->AGI = cap_min_max(total->AGI, -255, 255);
    capped->INT = cap_min_max(total->INT, -255, 255);
    capped->WIS = cap_min_max(total->WIS, -255, 255);
    capped->CHA = cap_min_max(total->CHA, -255, 255);
    capped->svMagic = cap_min_max(total->svMagic, -255, 255);
    capped->svFire = cap_min_max(total->svFire, -255, 255);
    capped->svCold = cap_min_max(total->svCold, -255, 255);
    capped->svPoison = cap_min_max(total->svPoison, -255, 255);
    capped->svDisease = cap_min_max(total->svDisease, -255, 255);
    
    /* Step 6: use capped stats to calc AC */
    mob->acFromAGI = mob_calc_ac_agi(level, capped->AGI);
    mob->acFromBonus = mob_calc_ac_bonus(level, raceId, classId, capped->AGI);
    mob->acFromSkill = mob_calc_ac_skill(classId, defense);
    mob->acFromSkill2 = mob_calc_ac_skill2(defense);
    mob->ac = mob_calc_ac_from_factors(mob, classId);
    
    /* Step 7: use capped stats to calc max hp and max mana */
    base->maxHp = client_calc_base_hp(classId, level, capped->STA);
    total->maxHp += base->maxHp;
    capped->maxHp = cap_min_max((int)total->maxHp, 1, INT16_MAX);
    
    base->maxMana = client_calc_base_mana(classId, level, capped->INT, capped->WIS);
    total->maxMana += base->maxMana;
    capped->maxMana = cap_min_max((int)total->maxMana, 0, INT16_MAX);
}

int64_t client_calc_base_hp(uint8_t classId, int level, int sta)
{
    int mult;
    
    switch (classId)
    {
    case CLASS_ID_Cleric:
    case CLASS_ID_Druid:
    case CLASS_ID_Shaman:
        mult = 15;
        break;
    
    case CLASS_ID_Necromancer:
    case CLASS_ID_Wizard:
    case CLASS_ID_Magician:
    case CLASS_ID_Enchanter:
        mult = 12;
        break;
    
    case CLASS_ID_Monk:
    case CLASS_ID_Bard:
    case CLASS_ID_Rogue:
        if (level < 51)
            mult = 18;
        else if (level < 58)
            mult = 19;
        else
            mult = 20;
        break;
        
    case CLASS_ID_Ranger:
        if (level < 58)
            mult = 20;
        else
            mult = 21;
        break;
        
    case CLASS_ID_Warrior:
        if (level < 20)
            mult = 22;
        else if (level < 30)
            mult = 23;
        else if (level < 40)
            mult = 25;
        else if (level < 53)
            mult = 27;
        else if (level < 57)
            mult = 28;
        else
            mult = 30;
        break;
        
    case CLASS_ID_Paladin:
    case CLASS_ID_ShadowKnight:
    default:
        if (level < 35)
            mult = 21;
        else if (level < 45)
            mult = 22;
        else if (level < 51)
            mult = 23;
        else if (level < 56)
            mult = 24;
        else if (level < 60)
            mult = 25;
        else
            mult = 26;
        break;
    }
    
    mult *= level;
    
    return 5 + mult + ((mult * sta) / 300);
}

int64_t client_calc_base_mana(uint8_t classId, int level, int INT, int WIS)
{
    int64_t ret;
    int softCapped = 0;
    
    switch (classId)
    {
    case CLASS_ID_ShadowKnight:
    case CLASS_ID_Bard:
    case CLASS_ID_Necromancer:
    case CLASS_ID_Wizard:
    case CLASS_ID_Magician:
    case CLASS_ID_Enchanter:
        if (INT > 200)
        {
            softCapped = INT - 200;
            INT = 200;
        }
        
        ret = ((INT / 5 + 2) * level) + (softCapped * 6);
        break;
    
    case CLASS_ID_Cleric:
    case CLASS_ID_Paladin:
    case CLASS_ID_Ranger:
    case CLASS_ID_Druid:
    case CLASS_ID_Shaman:
        if (WIS > 200)
        {
            softCapped = WIS - 200;
            WIS = 200;
        }
        
        ret = ((WIS / 5 + 2) * level) + (softCapped * 6);
        break;
    
    default:
        ret = 0;
        break;
    }
    
    return ret;
}

void client_on_unhandled_packet(Client* client, ToServerPacket* packet)
{
    Zone* zone = client_get_zone(client);
    lua_State* L = zone_lua(zone);
    
    zlua_event_prolog_literal("event_unhandled_packet", L, client_mob(client), zone);
    
    lua_createtable(L, 0, 4);
    lua_pushinteger(L, packet->opcode);
    lua_setfield(L, -2, "opcode");
    
    if (packet->data && packet->length)
    {
        lua_pushlstring(L, (const char*)packet->data, packet->length);
        lua_setfield(L, -2, "data");
    }
    
    zlua_event_epilog(L, zone, NULL);
}

void client_on_msg_command(Client* client, const char* msg, int len)
{
    zlua_event_command(client, msg, len);
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

int client_hp_from_items(Client* client)
{
    return client->hpFromItems;
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
    mob_set_target(mob, NULL);
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

void client_set_zone_index(Client* client, int index)
{
    mob_set_zone_index(&client->mob, index);
}

int client_zone_index(Client* client)
{
    return mob_zone_index(&client->mob);
}

void client_set_lua_index(Client* client, int index)
{
    mob_set_lua_index(&client->mob, index);
}

int client_lua_index(Client* client)
{
    return mob_lua_index(&client->mob);
}

int client_get_skill(Client* client, int skillId)
{
    return skill_get(&client->skills, skillId);
}

uint32_t client_increment_zone_in_count(Client* client)
{
    return client->zoneInCount++;
}

void client_update_hp(Client* client, int curHp)
{
    TlgPacket* packet;
    Mob* mob = client_mob(client);
    Zone* zone = mob_get_zone(mob);
    
    mob_set_cur_hp_no_cap_check(mob, (int64_t)curHp);
    
    packet = packet_create_hp_update_percentage(mob_entity_id(mob), mob_hp_ratio(mob));
    if (packet)
        zone_broadcast_to_all_clients(zone, packet); /* fixme: should we limit this to nearby + group members + anyone with this client targetted? */
    
    packet = packet_create_hp_update_self(client);
    if (packet)
        client_schedule_packet_with_zone(client, zone, packet);
}

void client_update_mana(Client* client, uint16_t curMana)
{
    TlgPacket* packet;
    Mob* mob = client_mob(client);
    
    mob_set_cur_mana_no_cap_check(mob, (int64_t)curMana);
    
    packet = packet_create_mana_update(curMana, 0xffff);
    if (packet)
        client_schedule_packet(client, packet);
}

void client_update_level(Client* client, uint8_t level)
{
    TlgPacket*  packet = packet_create_level_update(level);
    if (packet)
        client_schedule_packet(client, packet);

    mob_update_level(client_mob(client), level);
}

void client_update_exp(Client* client, uint32_t exp)
{
    TlgPacket* packet;

    client->experience = (int64_t)exp;

    packet = packet_create_exp_update(exp);
    if (packet)
        client_schedule_packet(client, packet);
}
