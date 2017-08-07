
#ifndef SKILLS_H
#define SKILLS_H

#include "define.h"
#include "aligned.h"

/* Languages are stores as skills in the DB, with this offset to their id numbers */
#define SKILL_LANGUAGE_DB_OFFSET 100

enum SkillId
{
    SKILL_1HBlunt,
    SKILL_1HSlashing,
    SKILL_2HBlunt,
    SKILL_2HSlashing,
    SKILL_Abjuration,
    SKILL_Alteration,
    SKILL_ApplyPoison,
    SKILL_Archery,
    SKILL_Backstab,
    SKILL_BindWound,
    SKILL_Bash,
    SKILL_Block,
    SKILL_BrassInstruments,
    SKILL_Channeling,
    SKILL_Conjuration,
    SKILL_Defense,
    SKILL_Disarm,
    SKILL_DisarmTraps,
    SKILL_Divination,
    SKILL_Dodge,
    SKILL_DoubleAttack,
    SKILL_DragonPunch,
    SKILL_DualWield,
    SKILL_EagleStrike,
    SKILL_Evocation,
    SKILL_FeignDeath,
    SKILL_FlyingKick,
    SKILL_Forage,
    SKILL_HandToHand,
    SKILL_Hide,
    SKILL_Kick,
    SKILL_Meditate,
    SKILL_Mend,
    SKILL_Offense,
    SKILL_Parry,
    SKILL_PickLock,
    SKILL_1HPiercing,
    SKILL_Riposte,
    SKILL_RoundKick,
    SKILL_SafeFall,
    SKILL_SenseHeading,
    SKILL_Singing,
    SKILL_Sneak,
    SKILL_SpecializeAbjuration,
    SKILL_SpecializeAlteration,
    SKILL_SpecializeConjuration,
    SKILL_SpecializeDivination,
    SKILL_SpecializeEvocation,
    SKILL_PickPockets,
    SKILL_StringedInstruments,
    SKILL_Swimming,
    SKILL_Throwing,
    SKILL_TigerClaw,
    SKILL_Tracking,
    SKILL_WindInstruments,
    SKILL_Fishing,
    SKILL_MakePoison,
    SKILL_Tinkering,
    SKILL_Research,
    SKILL_Alchemy,
    SKILL_Baking,
    SKILL_Tailoring,
    SKILL_SenseTraps,
    SKILL_Blacksmithing,
    SKILL_Fletching,
    SKILL_Brewing,
    SKILL_AlcoholTolerance,
    SKILL_Begging,
    SKILL_JewelryMaking,
    SKILL_Pottery,
    SKILL_PercussionInstruments,
    SKILL_Intimidation,
    SKILL_Berserking,
    SKILL_Taunt,
    SKILL_COUNT
};

enum LanguageId
{
    LANG_CommonTongue,
    LANG_Barbarian,
    LANG_Erudian,
    LANG_Elvish,
    LANG_DarkElvish,
    LANG_Dwarvish,
    LANG_Troll,
    LANG_Ogre,
    LANG_Gnomish,
    LANG_Halfling,
    LANG_TheivesCant,
    LANG_OldErudian,
    LANG_ElderElvish,
    LANG_Froglok,
    LANG_Goblin,
    LANG_Gnoll,
    LANG_CombineTongue,
    LANG_ElderTierDal,
    LANG_Lizardman,
    LANG_Orcish,
    LANG_Faerie,
    LANG_Dragon,
    LANG_ElderDragon,
    LANG_DarkSpeech,
    LANG_COUNT
};

typedef struct {
    uint8_t skill[SKILL_COUNT];
    uint8_t language[LANG_COUNT];
} Skills;

void skills_init(Skills* sk, uint8_t classId, uint8_t raceId, uint8_t level);
void skills_set_from_db(Skills* sk, uint32_t skillId, uint32_t value);
void skills_write_pp(Skills* sk, Aligned* a);
void skills_write_pp_languages(Skills* sk, Aligned* a);

#endif/*SKILLS_H*/
