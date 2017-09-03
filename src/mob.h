
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

struct Client;
struct Mob;

typedef struct Mob {
    uint8_t         parentType;
    uint8_t         level;
    uint8_t         classId;
    uint8_t         baseGenderId;
    uint8_t         genderId;
    uint8_t         faceId;
    uint16_t        baseRaceId;
    uint16_t        raceId;
    uint16_t        deityId;    /*fixme: map this down to 8 bits?*/
    int16_t         entityId;
    int             zoneIndex;
    int             luaIndex;
    LocH            loc;
    int16_t         headingRaw;
    StaticBuffer*   name;
    struct Mob*     target;
    /* Core stats */
    int64_t         currentHp;
    int64_t         currentMana;
    int64_t         currentEndurance;
    int             ac;
    int8_t          acFromAGI;
    int8_t          acFromBonus;
    int16_t         acFromSkill;
    int16_t         acFromSkill2;
    int             acFromItems;
    int             acFromBuffs;
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
    uint8_t         materialId[7];
    uint32_t        tint[7];
    int16_t         ownerEntityId;
    Zone*           zone;
} Mob;

void mob_init_client_unloaded(Mob* mob, StaticBuffer* name);
void mob_init_client_character(Mob* mob, ClientLoadData_Character* data);
void mob_deinit(Mob* mob);

int8_t mob_hp_ratio(Mob* mob);

int8_t mob_calc_ac_agi(int level, int AGI);
int8_t mob_calc_ac_bonus(int level, int raceId, int classId, int AGI);
int16_t mob_calc_ac_skill(int classId, int skillAmt);
int16_t mob_calc_ac_skill2(int skillAmt);
int mob_calc_ac_from_factors(Mob* mob, int classId);

ZEQ_API const char* mob_name_str(Mob* mob);
const char* mob_name_str_no_null(Mob* mob);
#define mob_set_name_sbuf(mob, sbuf) ((mob)->name = (sbuf))
#define mob_set_cur_hp_no_cap_check(mob, hp) ((mob)->currentHp = (hp))
ZEQ_API int64_t mob_cur_hp(Mob* mob);
ZEQ_API int64_t mob_max_hp(Mob* mob);
#define mob_set_cur_mana_no_cap_check(mob, mana) ((mob)->currentMana = (mana))
ZEQ_API int64_t mob_cur_mana(Mob* mob);
ZEQ_API int64_t mob_max_mana(Mob* mob);
#define mob_set_cur_endurance_no_cap_check(mob, end) ((mob)->currentEndurance = (end))

#define mob_base_stats(mob) (&((mob)->baseStats))
#define mob_total_stats(mob) (&((mob)->totalStats))
#define mob_capped_stats(mob) (&((mob)->cappedStats))

#define mob_set_level(mob, lvl) ((mob)->level = (lvl))
ZEQ_API uint8_t mob_level(Mob* mob);
ZEQ_API uint8_t mob_class_id(Mob* mob);
ZEQ_API uint8_t mob_gender_id(Mob* mob);
ZEQ_API void mob_update_gender_id(Mob* mob, uint8_t genderId);
ZEQ_API uint8_t mob_base_gender_id(Mob* mob);
ZEQ_API uint16_t mob_race_id(Mob* mob);
ZEQ_API void mob_update_race_id(Mob* mob, uint16_t raceId);
ZEQ_API uint16_t mob_base_race_id(Mob* mob);
#define mob_deity_id(mob) ((mob)->deityId)

ZEQ_API float mob_x(Mob* mob);
ZEQ_API float mob_y(Mob* mob);
ZEQ_API float mob_z(Mob* mob);

ZEQ_API Zone* mob_get_zone(Mob* mob);
#define mob_set_zone(mob, zone) ((mob)->zone = (zone))

#define mob_parent_type(mob) ((mob)->parentType)

ZEQ_API int16_t mob_entity_id(Mob* mob);
#define mob_set_entity_id(mob, id) ((mob)->entityId = (id))

#define mob_zone_index(mob) ((mob)->zoneIndex)
#define mob_set_zone_index(mob, idx) ((mob)->zoneIndex = (idx))
ZEQ_API int mob_lua_index(Mob* mob);
#define mob_set_lua_index(mob, idx) ((mob)->luaIndex = (idx))

ZEQ_API float mob_cur_size(Mob* mob);

#define mob_cur_walking_speed(mob) ((mob)->currentWalkingSpeed)
#define mob_cur_running_speed(mob) ((mob)->currentRunningSpeed)

ZEQ_API uint8_t mob_texture_id(Mob* mob);
ZEQ_API void mob_update_texture_id(Mob* mob, uint8_t textureId);
ZEQ_API uint8_t mob_helm_texture_id(Mob* mob);
ZEQ_API void mob_update_helm_texture_id(Mob* mob, uint8_t textureId);
#define mob_face_id(mob) ((mob)->faceId)
#define mob_primary_weapon_model_id(mob) ((mob)->primaryWeaponModelId)
#define mob_secondary_weapon_model_id(mob) ((mob)->secondaryWeaponModelId)
#define mob_upright_state(mob) ((mob)->uprightState)
#define mob_set_upright_state(mob, state) ((mob)->uprightState = (state))
#define mob_light_level(mob) ((mob)->lightLevel)
#define mob_body_type(mob) ((mob)->bodyType)
ZEQ_API uint8_t mob_material_id(Mob* mob, int n);
void mob_set_material_id(Mob* mob, int n, uint8_t matId);
ZEQ_API void mob_update_material_id(Mob* mob, int n, uint8_t matId);
ZEQ_API uint32_t mob_tint(Mob* mob, int n);
void mob_set_tint(Mob* mob, int n, uint32_t color);
ZEQ_API void mob_update_tint(Mob* mob, int n, uint32_t color);
void mob_update_material_and_tint(Mob* mob, int n, uint8_t matId, uint32_t color);
#define mob_owner_entity_id(mob) ((mob)->ownerEntityId)

#define mob_as_client(mob) (struct Client*)(mob)

#define mob_set_target(mob, targ) ((mob)->target = (targ))
ZEQ_API Mob* mob_target(Mob* mob);
Mob* mob_target_or_self(Mob* mob);

ZEQ_API void mob_update_level(Mob* mob, uint8_t level);
ZEQ_API void mob_update_size(Mob* mob, float size);

ZEQ_API void mob_animate_nearby(Mob* mob, uint32_t animId);
ZEQ_API void mob_animate_range(Mob* mob, uint32_t animId, double range);

Timer* mob_create_timer(Mob* mob, uint32_t periodMs, TimerCallback callback, void* userdata, bool start);
Timer* mob_destroy_timer(Mob* mob, Timer* timer);

#endif/*MOB_H*/
