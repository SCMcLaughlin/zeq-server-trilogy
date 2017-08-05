
#ifndef MOB_H
#define MOB_H

#include "define.h"
#include "buffer.h"
#include "loc.h"
#include "misc_struct.h"

enum MobParentType
{
    MOB_PARENT_TYPE_NONE,
    MOB_PARENT_TYPE_NPC,
    MOB_PARENT_TYPE_Client,
};

typedef struct {
    uint8_t         parentType;
    uint8_t         level;
    uint8_t         classId;
    uint8_t         genderId;
    uint8_t         faceId;
    uint8_t         unused_1;
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
    CoreStats       totalStats; /* What their stats would be if caps did not apply, used to simplify calculations */
} Mob;

void mob_deinit(Mob* mob);

#define mob_set_name_sbuf(mob, sbuf) ((mob)->name = (sbuf))
#define mob_set_cur_hp_no_cap_check(mob, hp) ((mob)->currentHp = (hp))
#define mob_set_cur_mana_no_cap_check(mob, mana) ((mob)->currentMana = (mana))
#define mob_set_cur_endurance_no_cap_check(mob, end) ((mob)->currentEndurance = (end))

#endif/*MOB_H*/
