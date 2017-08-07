
#ifndef MOB_H
#define MOB_H

#include "define.h"
#include "buffer.h"
#include "client_load_data.h"
#include "loc.h"
#include "misc_struct.h"
#include "zone.h"

enum MobParentType
{
    MOB_PARENT_TYPE_NONE,
    MOB_PARENT_TYPE_Npc,
    MOB_PARENT_TYPE_Client,
    MOB_PARENT_TYPE_Pet,
    MOB_PARENT_TYPE_NpcCorpse,
    MOB_PARENT_TYPE_ClientCorpse,
};

typedef struct {
    uint8_t         parentType;
    uint8_t         level;
    uint8_t         classId;
    uint8_t         baseGenderId;
    uint8_t         genderId;
    uint8_t         faceId;
    uint16_t        baseRaceId;
    uint16_t        raceId;
    uint16_t        deityId;    /*fixme: map this down to 8 bits?*/
    LocH            loc;
    StaticBuffer*   name;
    /* Core stats */
    int64_t         currentHp;
    int64_t         currentMana;
    int64_t         currentEndurance;
    CoreStats       cappedStats;    /* The stats to use for most purposes */
    CoreStats       baseStats;
    CoreStats       totalStats;     /* What their stats would be if caps did not apply, used to simplify calculations */
    float           currentWalkingSpeed;
    float           currentRunningSpeed;
    float           baseWalkingSpeed;
    float           baseRunningSpeed;
    float           currentSize;
    float           baseSize;
    uint8_t         textureId;
    uint8_t         helmTextureId;
    uint8_t         primaryWeaponModelId;
    uint8_t         secondaryWeaponModelId;
    uint8_t         uprightState;
    uint8_t         lightLevel;
    uint8_t         bodyType;
    Zone*           zone;
} Mob;

void mob_init_client_unloaded(Mob* mob, StaticBuffer* name);
void mob_init_client_character(Mob* mob, ClientLoadData_Character* data);
void mob_deinit(Mob* mob);

#define mob_set_name_sbuf(mob, sbuf) ((mob)->name = (sbuf))
#define mob_set_cur_hp_no_cap_check(mob, hp) ((mob)->currentHp = (hp))
#define mob_set_cur_mana_no_cap_check(mob, mana) ((mob)->currentMana = (mana))
#define mob_set_cur_endurance_no_cap_check(mob, end) ((mob)->currentEndurance = (end))

#define mob_get_zone(mob) ((mob)->zone)
#define mob_set_zone(mob, zone) ((mob)->zone = (zone))

#endif/*MOB_H*/
