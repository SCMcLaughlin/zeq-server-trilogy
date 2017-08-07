
#include "skills.h"
#include "class_id.h"
#include "player_profile_packet_struct.h"
#include "race_id.h"

#define SKILL_CAP 250
#define SKILL_CANT_LEARN 0xff
#define SKILL_HAVENT_TRAINED_YET 0xfe
#define LANG_CAP 100

void skills_init(Skills* sk, uint8_t classId, uint8_t raceId, uint8_t level)
{
    uint32_t i;
    
    /*fixme: do we need this here?*/
    (void)level;
    
    static const uint8_t skillsThatDefaultToZeroUniversally[] = {
        SKILL_1HBlunt,
        SKILL_BindWound,
        SKILL_Defense,
        SKILL_HandToHand,
        SKILL_Offense,
        SKILL_SenseHeading,
        SKILL_Swimming, /* Except for Iksars, which will be handled further down */
        SKILL_Throwing,
        SKILL_Fishing,
        SKILL_Baking,
        SKILL_Tailoring,
        SKILL_Blacksmithing,
        SKILL_Fletching,
        SKILL_Brewing,
        SKILL_AlcoholTolerance,
        SKILL_Begging,
        SKILL_JewelryMaking,
        SKILL_Pottery
    };
    
    memset(sk->skill, SKILL_HAVENT_TRAINED_YET, sizeof(sk->skill));
    memset(sk->language, 0, sizeof(sk->language));
    
    for (i = 0; i < sizeof(skillsThatDefaultToZeroUniversally); i++)
    {
        sk->skill[skillsThatDefaultToZeroUniversally[i]] = 0;
    }
    
    sk->language[LANG_CommonTongue] = LANG_CAP;
    
    if (classId != CLASS_ID_Monk)
    {
        sk->skill[SKILL_Block] = SKILL_CANT_LEARN;
        sk->skill[SKILL_DragonPunch] = SKILL_CANT_LEARN;
        sk->skill[SKILL_EagleStrike] = SKILL_CANT_LEARN;
        sk->skill[SKILL_FeignDeath] = SKILL_CANT_LEARN;
        sk->skill[SKILL_FlyingKick] = SKILL_CANT_LEARN;
        sk->skill[SKILL_RoundKick] = SKILL_CANT_LEARN;
        sk->skill[SKILL_TigerClaw] = SKILL_CANT_LEARN;
    }
    
    if (classId != CLASS_ID_Rogue)
    {
        sk->skill[SKILL_ApplyPoison] = SKILL_CANT_LEARN;
        sk->skill[SKILL_Backstab] = SKILL_CANT_LEARN;
        sk->skill[SKILL_DisarmTraps] = SKILL_CANT_LEARN;
        sk->skill[SKILL_PickLock] = SKILL_CANT_LEARN;
        sk->skill[SKILL_PickPockets] = SKILL_CANT_LEARN;
        sk->skill[SKILL_MakePoison] = SKILL_CANT_LEARN;
    }
    
    if (classId < CLASS_ID_Necromancer || classId > CLASS_ID_Enchanter)
        sk->skill[SKILL_Research] = SKILL_CANT_LEARN;
    
    if (classId != CLASS_ID_Shaman)
        sk->skill[SKILL_Alchemy] = SKILL_CANT_LEARN;
    
    if (raceId != RACE_ID_Gnome)
        sk->skill[SKILL_Tinkering] = SKILL_CANT_LEARN;
    
    if (raceId == RACE_ID_Iksar)
        sk->skill[SKILL_Swimming] = 100;
}

void skills_set_from_db(Skills* sk, uint32_t skillId, uint32_t value)
{
    if (skillId >= SKILL_LANGUAGE_DB_OFFSET)
    {
        skillId -= SKILL_LANGUAGE_DB_OFFSET;
        
        if (skillId >= LANG_COUNT)
            return;
        
        sk->language[skillId] = (uint8_t)value;
    }
    
    sk->skill[skillId] = (uint8_t)value;
}

void skills_write_pp(Skills* sk, Aligned* a)
{
    uint32_t i;
    
    assert(SKILL_COUNT == ZEQ_PP_SKILLS_COUNT);
    
    for (i = 0; i < SKILL_COUNT; i++)
    {
        aligned_write_uint8(a, sk->skill[i]);
    }
}

void skills_write_pp_languages(Skills* sk, Aligned* a)
{
    uint32_t i;
    
    assert(LANG_COUNT == ZEQ_PP_LANGUAGES_COUNT);
    
    for (i = 0; i < LANG_COUNT; i++)
    {
        aligned_write_uint8(a, sk->language[i]);
    }
}
