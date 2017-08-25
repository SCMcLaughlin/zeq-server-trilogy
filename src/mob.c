
#include "mob.h"
#include "buffer.h"
#include "class_id.h"
#include "client_load_data.h"
#include "loc.h"
#include "misc_enum.h"
#include "misc_struct.h"
#include "packet_create.h"
#include "race_id.h"
#include "util_cap.h"
#include "zone.h"

void mob_init_client_unloaded(Mob* mob, StaticBuffer* name)
{
    mob->parentType = MOB_PARENT_TYPE_Client;
    mob->name = name;
    sbuf_grab(name);
}

static float mob_base_size_by_race_id(int raceId)
{
    float ret;
    
    switch (raceId)
    {
    case RACE_ID_Barbarian:
    case RACE_ID_Ogre:
    case RACE_ID_Troll:
        ret = 6.0f;
        break;
    
    case RACE_ID_WoodElf:
    case RACE_ID_DarkElf:
        ret = 4.0f;
        break;
    
    case RACE_ID_Dwarf:
    case RACE_ID_Halfling:
        ret = 3.0f;
        break;
    
    case RACE_ID_Gnome:
        ret = 2.5f;
        break;
    
    default:
        ret = 5.0f;
        break;
    }
    
    return ret;
}

void mob_init_client_character(Mob* mob, ClientLoadData_Character* data)
{
    float size;
    float runSpeed;
    
    mob->level = data->level;
    mob->classId = data->classId;
    mob->baseGenderId = data->genderId;
    mob->genderId = data->genderId;
    mob->faceId = data->faceId;
    mob->baseRaceId = data->raceId;
    mob->raceId = data->raceId;
    mob->deityId = data->deityId;
    mob->zoneIndex = -1;
    mob->loc = data->loc;
    mob_set_cur_hp_no_cap_check(mob, data->currentHp);
    mob_set_cur_mana_no_cap_check(mob, data->currentMana);
    mob_set_cur_endurance_no_cap_check(mob, data->currentEndurance);
    mob->baseStats.STR = data->STR;
    mob->baseStats.STA = data->STA;
    mob->baseStats.DEX = data->DEX;
    mob->baseStats.AGI = data->AGI;
    mob->baseStats.INT = data->INT;
    mob->baseStats.WIS = data->WIS;
    mob->baseStats.CHA = data->CHA;
    
    mob->textureId = 0xff;
    mob->helmTextureId = 0xff;
    mob->primaryWeaponModelId = 0;
    mob->secondaryWeaponModelId = 0;
    mob->uprightState = UPRIGHT_STATE_Standing;
    mob->lightLevel = 0;
    mob->bodyType = BODY_TYPE_Humanoid;
    
    /* Temporary, just to make sure we don't try to divide by zero anywhere */
    mob->baseStats.maxHp = 100;
    mob->baseStats.maxMana = 100;
    mob->baseStats.maxEndurance = 100;
    mob->cappedStats.maxHp = 100;
    mob->cappedStats.maxMana = 100;
    mob->cappedStats.maxEndurance = 100;
    
    runSpeed = (data->isGMSpeed) ? 3.0f : 0.7f;
    
    mob->currentWalkingSpeed = 0.46f;
    mob->currentRunningSpeed = runSpeed;
    mob->baseWalkingSpeed = 0.46f;
    mob->baseRunningSpeed = runSpeed;
    
    size = mob_base_size_by_race_id((int)mob->baseRaceId);
    
    mob->currentSize = size;
    mob->baseSize = size;
}

void mob_deinit(Mob* mob)
{
    mob->name = sbuf_drop(mob->name);
}

int8_t mob_hp_ratio(Mob* mob)
{
    return (int8_t)((mob->currentHp * 100) / mob->cappedStats.maxHp);
}

int8_t mob_calc_ac_agi(int level, int AGI)
{
    int8_t val = 0;
    
    if (AGI <= 74)
    {
        if (AGI < 0)
            AGI = 0;
        
        static const int8_t byLowAgi[75] = {
            -24, -24, -23, -23, -22, -21, -21, -20, -20, -19,   /* 0-9 */
            -18, -18, -17, -16, -16, -15, -15, -14, -13, -13,   /* 10-19 */
            -12, -11, -11, -10, -10,  -9,  -8,  -8,  -7,  -6,   /* 20-29 */
             -6,  -5,  -5,  -4,  -3,  -3,  -2,  -1,  -1,   0,   /* 30-39 */
              0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   /* 40-49 */
              0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   /* 50-59 */
              0,   0,   0,   0,   0,   0,   1,   1,   1,   1,   /* 60-69 */
              1,   5,   5,   5,   5                             /* 70-74 */
        };
        
        val = byLowAgi[AGI];
    }
    else
    {
        if (AGI <= 75)
            AGI = 0;
        else if (AGI <= 79)
            AGI = 1;
        else if (AGI <= 80)
            AGI = 2;
        else if (AGI <= 85)
            AGI = 3;
        else if (AGI <= 90)
            AGI = 4;
        else if (AGI <= 95)
            AGI = 5;
        else if (AGI <= 99)
            AGI = 6;
        else if (AGI <= 100)
            AGI = 7;
        else if (AGI <= 105)
            AGI = 8;
        else if (AGI <= 110)
            AGI = 9;
        else if (AGI <= 115)
            AGI = 10;
        else if (AGI <= 119)
            AGI = 11;
        else if (AGI <= 120)
            AGI = 12;
        else if (AGI <= 125)
            AGI = 13;
        else if (AGI <= 130)
            AGI = 14;
        else if (AGI <= 135)
            AGI = 15;
        else if (AGI <= 139)
            AGI = 16;
        else if (AGI <= 140)
            AGI = 17;
        else if (AGI <= 145)
            AGI = 18;
        else if (AGI <= 150)
            AGI = 19;
        else if (AGI <= 155)
            AGI = 20;
        else if (AGI <= 159)
            AGI = 21;
        else if (AGI <= 160)
            AGI = 22;
        else if (AGI <= 165)
            AGI = 23;
        else if (AGI <= 170)
            AGI = 24;
        else if (AGI <= 175)
            AGI = 25;
        else if (AGI <= 179)
            AGI = 26;
        else if (AGI <= 180)
            AGI = 27;
        else if (AGI <= 185)
            AGI = 28;
        else if (AGI <= 190)
            AGI = 29;
        else if (AGI <= 195)
            AGI = 30;
        else if (AGI <= 199)
            AGI = 31;
        else if (AGI <= 219)
            AGI = 32;
        else if (AGI <= 239)
            AGI = 33;
        else
            AGI = 34;
        
        if (level <= 6)
            level = 0;
        else if (level <= 19)
            level = 1;
        else if (level <= 39)
            level = 2;
        else
            level = 3;
        
        static const int8_t byHighAgiAndLevel[35][4] = {
            {  9, 23, 33, 39 },
            { 10, 23, 33, 40 },
            { 11, 24, 34, 41 },
            { 12, 25, 35, 42 },
            { 12, 26, 36, 42 },
            { 13, 26, 36, 43 },
            { 14, 27, 37, 44 },
            { 14, 28, 38, 45 },
            { 15, 29, 39, 45 },
            { 16, 29, 39, 46 },
            { 17, 30, 40, 47 },
            { 17, 31, 41, 47 },
            { 18, 32, 42, 48 },
            { 19, 32, 42, 49 },
            { 20, 33, 43, 50 },
            { 20, 34, 44, 50 },
            { 21, 34, 44, 51 },
            { 22, 35, 45, 52 },
            { 23, 36, 46, 53 },
            { 23, 37, 47, 53 },
            { 24, 37, 47, 54 },
            { 25, 38, 48, 55 },
            { 26, 39, 49, 56 },
            { 26, 40, 50, 56 },
            { 27, 40, 50, 57 },
            { 28, 41, 51, 58 },
            { 28, 42, 52, 58 },
            { 29, 43, 53, 59 },
            { 30, 43, 53, 60 },
            { 31, 44, 54, 61 },
            { 31, 45, 55, 61 },
            { 32, 45, 55, 62 },
            { 33, 46, 56, 63 },
            { 34, 47, 57, 64 },
            { 35, 48, 58, 65 }
        };
        
        val = byHighAgiAndLevel[AGI][level];
    }
    
    return val;
}

static int8_t mob_calc_iksar_ac_bonus(int level)
{
    return cap_min_max(level, 10, 35);
}

static int8_t mob_calc_monk_ac_bonus(int level)
{
    static const int8_t bonusByLevel[60] = {
         7,  9, 10, 11, 13, 14, 15, 17, 18, 19,
        21, 22, 23, 25, 26, 27, 29, 30, 31, 33,
        34, 35, 37, 38, 39, 41, 42, 43, 45, 46,
        47, 49, 50, 51, 53, 54, 55, 57, 58, 59,
        61, 62, 63, 65, 66, 67, 69, 70, 71, 73,
        74, 75, 77, 78, 79, 81, 82, 83, 85, 86
    };
    
    level--;
    
    if (level >= 60)
        level = 59;
    
    return bonusByLevel[level];
}

static int8_t mob_calc_rogue_ac_bonus(int level, int AGI)
{
    int8_t val = 0;
    
    if (level >= 30 && AGI >= 76)
    {
        level -= 30;
        
        if (level > 30)
            level = 30;
        
        if (AGI <= 79)
            AGI = 0;
        else if (AGI <= 84)
            AGI = 1;
        else if (AGI <= 89)
            AGI = 2;
        else if (AGI <= 99)
            AGI = 3;
        else
            AGI = 4;
        
        /* The progression of these numbers seems curve-like just looking at them, but I don't have the patience to work backwards to the formula */
        static const int8_t bonusByAgiAndLevel[5][31] = {
            { 1,  1,  1,  1,  2,  2,  2,  2,  3,  3,  3,  3,  4,  4,  4,  4,  5,  5,  5,  5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8 },
            { 2,  2,  3,  3,  4,  4,  5,  5,  6,  6,  7,  7,  8,  8,  9,  9, 10, 10, 11, 11, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12 },
            { 3,  3,  4,  5,  6,  6,  7,  8,  9,  9, 10, 11, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12 },
            { 4,  5,  6,  7,  8,  9, 10, 11, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12 },
            { 5,  6,  7,  8, 10, 11, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12 } 
        };
        
        val = bonusByAgiAndLevel[AGI][level];
    }
    
    return val;
}

int8_t mob_calc_ac_bonus(int level, int raceId, int classId, int AGI)
{
    int8_t val = 0;
    
    if (raceId == RACE_ID_Iksar)
        val += mob_calc_iksar_ac_bonus(level);
    
    switch (classId)
    {
    case CLASS_ID_Monk:
        val += mob_calc_monk_ac_bonus(level);
        break;
    
    case CLASS_ID_Rogue:
        val += mob_calc_rogue_ac_bonus(level, AGI);
        break;
    
    default:
        break;
    }
    
    return val;
}

int16_t mob_calc_ac_skill(int classId, int skillAmt)
{
    int16_t val;
    
    switch (classId)
    {
    case CLASS_ID_Necromancer:
    case CLASS_ID_Wizard:
    case CLASS_ID_Magician:
    case CLASS_ID_Enchanter:
        val = (int16_t)(skillAmt / 2);
        break;
    
    default:
        val = (int16_t)(skillAmt / 3);
        break;
    }
    
    return val;
}

int16_t mob_calc_ac_skill2(int skillAmt)
{
    return skillAmt * 16 / 9;
}

int mob_calc_ac_from_factors(Mob* mob, int classId)
{
    int avoidance = mob->acFromAGI + mob->acFromBonus + mob->acFromSkill2;
    int mitigation = mob->acFromSkill;
    
    switch (classId)
    {
    case CLASS_ID_Necromancer:
    case CLASS_ID_Wizard:
    case CLASS_ID_Magician:
    case CLASS_ID_Enchanter:
        mitigation += mob->acFromItems;
        break;
    
    default:
        mitigation += mob->acFromItems * 4 / 3;
        break;
    }
    
    if (avoidance < 0)
        avoidance = 0;
    
    if (mitigation < 0)
        mitigation = 0;
    
    return ((avoidance + mitigation) * 1000 / 847) + mob->acFromBuffs;
}

const char* mob_name_str(Mob* mob)
{
    return sbuf_str((mob)->name);
}

int64_t mob_cur_hp(Mob* mob)
{
    return mob->currentHp;
}

int64_t mob_max_hp(Mob* mob)
{
    return mob->cappedStats.maxHp;
}

int64_t mob_cur_mana(Mob* mob)
{
    return mob->currentMana;
}

uint8_t mob_level(Mob* mob)
{
    return mob->level;
}

float mob_x(Mob* mob)
{
    return mob->loc.x;
}

float mob_y(Mob* mob)
{
    return mob->loc.y;
}

float mob_z(Mob* mob)
{
    return mob->loc.z;
}

Zone* mob_get_zone(Mob* mob)
{
    return mob->zone;
}

int mob_lua_index(Mob* mob)
{
    return mob->luaIndex;
}

double mob_cur_size(Mob* mob)
{
    return mob->currentSize;
}

Mob* mob_target(Mob* mob)
{
    return mob->target;
}

Mob* mob_target_or_self(Mob* mob)
{
    Mob* targ = mob->target;
    return (targ) ? targ : mob;
}

void mob_update_size(Mob* mob, double size)
{
    TlgPacket* packet;
    Zone* zone;
    
    mob->currentSize = (float)size;
    zone = mob_get_zone(mob);
    packet = packet_create_spawn_appearance(mob_entity_id(mob), SPAWN_APPEARANCE_Size, (int)size);
    
    if (packet)
        zone_broadcast_to_all_clients(zone, packet);
}

void mob_update_level(Mob* mob, uint8_t level)
{
    Zone* zone = mob_get_zone(mob);
    TlgPacket* packet;

    mob_set_level(mob, level);

    packet = packet_create_spawn_appearance(mob_entity_id(mob), SPAWN_APPEARANCE_LevelChange, (int)level);
    if (packet)
        zone_broadcast_to_all_clients(zone, packet);
}
