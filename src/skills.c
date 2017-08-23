
#include "skills.h"
#include "class_id.h"
#include "player_profile_packet_struct.h"
#include "race_id.h"

#define SKILL_CAP 250
#define SKILL_CANT_LEARN 0xff
#define SKILL_HAVENT_TRAINED_YET 0xfe
#define LANG_CAP 100

#define RACIAL_HIDE_INDEX 0
#define RACIAL_SNEAK_INDEX 1
#define RACIAL_FORAGE_INDEX 2
#define RACIAL_SWIMMING_INDEX 3
#define RACIAL_TINKERING_INDEX 4

void skills_init(Skills* sk, uint8_t classId, uint16_t raceId)
{
    uint32_t i;
    uint32_t raceIndex = raceId - 1;
    
    if (raceIndex > 12)
        raceIndex = 12;
    
    classId--;
    
    if (classId > 13)
        classId = 13;

#define CANT SKILL_CANT_LEARN
#define NOTYET SKILL_HAVENT_TRAINED_YET
    
    static const uint8_t racialSkills[13][5] = {
    /*    Hide Sneak Forage Swimming Tinkering */
        { CANT, CANT,  CANT,    CANT,     CANT }, /* Human     */
        { CANT, CANT,  CANT,    CANT,     CANT }, /* Barbarian */
        { CANT, CANT,  CANT,    CANT,     CANT }, /* Erudite   */
        {   50, CANT,    50,    CANT,     CANT }, /* Wood Elf  */
        { CANT, CANT,  CANT,    CANT,     CANT }, /* High Elf  */
        {   50, CANT,  CANT,    CANT,     CANT }, /* Dark Elf  */
        { CANT, CANT,  CANT,    CANT,     CANT }, /* Half Elf  */
        { CANT, CANT,  CANT,    CANT,     CANT }, /* Dwarf     */
        { CANT, CANT,  CANT,    CANT,     CANT }, /* Troll     */
        { CANT, CANT,  CANT,    CANT,     CANT }, /* Ogre      */
        {   50,   50,  CANT,    CANT,     CANT }, /* Halfling  */
        { CANT, CANT,  CANT,    CANT,       50 }, /* Gnome     */
        { CANT, CANT,    50,     100,     CANT }  /* Iksar     */
    };
    
    static const uint8_t classLvl1Skills[SKILL_COUNT][14] = {
    /*       WAR     CLR     PAL     RNG     SHD     DRU     MNK     BRD     ROG     SHM     NEC     WIZ     MAG     ENC */
        {      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0 }, /* 1HBlunt */
        {      0,   CANT,      0,      0,      0,      0,   CANT,      0,      0,   CANT,   CANT,   CANT,   CANT,   CANT }, /* 1HSlashing */
        {      0,      0,      0,      0,      0,      0,      0,   CANT,   CANT,      0,      0,      0,      0,      0 }, /* 2HBlunt */
        {      0,   CANT,      0,      0,      0,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT }, /* 2HSlashing */
        {   CANT,      0, NOTYET, NOTYET, NOTYET,      0,   CANT,   CANT,   CANT,      0,      0,      0,      0,      0 }, /* Abjuration */
        {   CANT,      0, NOTYET, NOTYET, NOTYET,      0,   CANT,   CANT,   CANT,      0,      0,      0,      0,      0 }, /* Alteration */
        {   CANT,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT, NOTYET,   CANT,   CANT,   CANT,   CANT,   CANT }, /* ApplyPoison */
        {      0,   CANT,      0,      0,      0,   CANT,   CANT,   CANT,      0,   CANT,   CANT,   CANT,   CANT,   CANT }, /* Archery */
        {   CANT,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT, NOTYET,   CANT,   CANT,   CANT,   CANT,   CANT }, /* Backstab */
        {      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0 }, /* BindWound */
        {      0,   CANT,      0,   CANT,      0,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT }, /* Bash */
        {   CANT,   CANT,   CANT,   CANT,   CANT,   CANT, NOTYET,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT }, /* Block */
        {   CANT,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT,      0,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT }, /* BrassInstruments */
        {   CANT, NOTYET, NOTYET, NOTYET, NOTYET, NOTYET,   CANT,   CANT,   CANT, NOTYET,      0,      0,      0,      0 }, /* Channeling */
        {   CANT,      0, NOTYET, NOTYET, NOTYET,      0,   CANT,   CANT,   CANT,      0,      0,      0,      0,      0 }, /* Conjuration */
        {      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0 }, /* Defense */
        { NOTYET,   CANT, NOTYET, NOTYET, NOTYET,   CANT, NOTYET,   CANT, NOTYET,   CANT,   CANT,   CANT,   CANT,   CANT }, /* Disarm */
        {   CANT,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT, NOTYET, NOTYET,   CANT,   CANT,   CANT,   CANT,   CANT }, /* DisarmTraps */
        {   CANT,      0, NOTYET, NOTYET, NOTYET,      0,   CANT,   CANT,   CANT,      0,      0,      0,      0,      0 }, /* Divination */
        { NOTYET, NOTYET, NOTYET, NOTYET, NOTYET, NOTYET,      0, NOTYET, NOTYET, NOTYET, NOTYET, NOTYET, NOTYET, NOTYET }, /* Dodge */
        { NOTYET,   CANT, NOTYET, NOTYET, NOTYET,   CANT, NOTYET,   CANT, NOTYET,   CANT,   CANT,   CANT,   CANT,   CANT }, /* DoubleAttack */
        {   CANT,   CANT,   CANT,   CANT,   CANT,   CANT, NOTYET,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT }, /* DragonPunch */
        { NOTYET,   CANT,   CANT, NOTYET,   CANT,   CANT,      0, NOTYET, NOTYET,   CANT,   CANT,   CANT,   CANT,   CANT }, /* DualWield */
        {   CANT,   CANT,   CANT,   CANT,   CANT,   CANT, NOTYET,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT }, /* EagleStrike */
        {   CANT,      0, NOTYET, NOTYET, NOTYET,      0,   CANT,   CANT,   CANT,      0,      0,      0,      0,      0 }, /* Evocation */
        {   CANT,   CANT,   CANT,   CANT,   CANT,   CANT, NOTYET,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT }, /* FeignDeath */
        {   CANT,   CANT,   CANT,   CANT,   CANT,   CANT, NOTYET,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT }, /* FlyingKick */
        {   CANT,   CANT,   CANT, NOTYET,   CANT, NOTYET,   CANT, NOTYET,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT }, /* Forage */
        {      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0 }, /* HandToHand */
        {   CANT,   CANT,   CANT, NOTYET, NOTYET,   CANT,   CANT, NOTYET, NOTYET,   CANT,   CANT,   CANT,   CANT,   CANT }, /* Hide */
        {      0,   CANT,   CANT, NOTYET,   CANT,   CANT,      0,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT }, /* Kick */
        {   CANT, NOTYET, NOTYET, NOTYET, NOTYET, NOTYET,   CANT,   CANT,   CANT, NOTYET, NOTYET, NOTYET, NOTYET, NOTYET }, /* Meditate */
        {   CANT,   CANT,   CANT,   CANT,   CANT,   CANT,      0,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT }, /* Mend */
        {      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0 }, /* Offense */
        { NOTYET,   CANT, NOTYET, NOTYET, NOTYET,   CANT,   CANT, NOTYET, NOTYET,   CANT,   CANT,   CANT,   CANT,   CANT }, /* Parry */
        {   CANT,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT, NOTYET, NOTYET,   CANT,   CANT,   CANT,   CANT,   CANT }, /* PickLock */
        {      0,   CANT,      0,      0,      0,   CANT,   CANT,      0,      0,      0,      0,      0,      0,      0 }, /* 1HPiercing */
        { NOTYET,   CANT, NOTYET, NOTYET, NOTYET,   CANT, NOTYET, NOTYET, NOTYET,   CANT,   CANT,   CANT,   CANT,   CANT }, /* Riposte */
        {   CANT,   CANT,   CANT,   CANT,   CANT,   CANT, NOTYET,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT }, /* RoundKick */
        {   CANT,   CANT,   CANT,   CANT,   CANT,   CANT, NOTYET, NOTYET, NOTYET,   CANT,   CANT,   CANT,   CANT,   CANT }, /* SafeFall */
        {      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0 }, /* SenseHeading */
        {   CANT,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT,      0,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT }, /* Singing */
        {   CANT,   CANT,   CANT, NOTYET,   CANT,   CANT, NOTYET, NOTYET,      0,   CANT,   CANT,   CANT,   CANT,   CANT }, /* Sneak */
        {   CANT, NOTYET,   CANT,   CANT,   CANT, NOTYET,   CANT,   CANT,   CANT, NOTYET, NOTYET, NOTYET, NOTYET, NOTYET }, /* SpecializeAbjuration */
        {   CANT, NOTYET,   CANT,   CANT,   CANT, NOTYET,   CANT,   CANT,   CANT, NOTYET, NOTYET, NOTYET, NOTYET, NOTYET }, /* SpecializeAlteration */
        {   CANT, NOTYET,   CANT,   CANT,   CANT, NOTYET,   CANT,   CANT,   CANT, NOTYET, NOTYET, NOTYET, NOTYET, NOTYET }, /* SpecializeConjuration */
        {   CANT, NOTYET,   CANT,   CANT,   CANT, NOTYET,   CANT,   CANT,   CANT, NOTYET, NOTYET, NOTYET, NOTYET, NOTYET }, /* SpecializeDivination */
        {   CANT, NOTYET,   CANT,   CANT,   CANT, NOTYET,   CANT,   CANT,   CANT, NOTYET, NOTYET, NOTYET, NOTYET, NOTYET }, /* SpecializeEvocation */
        {   CANT,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT, NOTYET,   CANT,   CANT,   CANT,   CANT,   CANT }, /* PickPockets */
        {   CANT,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT,      0,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT }, /* StringedInstruments */
        {      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0 }, /* Swimming */
        {      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0 }, /* Throwing */
        {   CANT,   CANT,   CANT,   CANT,   CANT,   CANT, NOTYET,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT }, /* TigerClaw */
        {   CANT,   CANT,   CANT,      0,   CANT, NOTYET,   CANT, NOTYET,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT }, /* Tracking */
        {   CANT,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT,      0,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT }, /* WindInstruments */
        {      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0 }, /* Fishing */
        {   CANT,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT, NOTYET,   CANT,   CANT,   CANT,   CANT,   CANT }, /* MakePoison */
        {   CANT,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT }, /* Tinkering */
        {   CANT,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT, NOTYET, NOTYET, NOTYET, NOTYET }, /* Research */
        {   CANT,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT, NOTYET,   CANT,   CANT,   CANT,   CANT }, /* Alchemy */
        {      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0 }, /* Baking */
        {      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0 }, /* Tailoring */
        {   CANT,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT, NOTYET, NOTYET,   CANT,   CANT,   CANT,   CANT,   CANT }, /* SenseTraps */
        {      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0 }, /* Blacksmithing */
        {      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0 }, /* Fletching */
        {      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0 }, /* Brewing */
        {      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0 }, /* AlcoholTolerance */
        {      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0 }, /* Begging */
        {      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0 }, /* JewelryMaking */
        {      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0 }, /* Pottery */
        {   CANT,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT,      0,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT }, /* PercussionInstruments */
        {   CANT,   CANT,   CANT,   CANT,   CANT,   CANT, NOTYET, NOTYET,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT }, /* Intimidation */
        {   CANT,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT }, /* Berserking (?) */
        {      0,   CANT,      0,      0,      0,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT,   CANT }  /* Taunt */
        /*   WAR     CLR     PAL     RNG     SHD     DRU     MNK     BRD     ROG     SHM     NEC     WIZ     MAG     ENC */
    };

#undef CANT
#undef NOTYET
    
    memset(sk->skill, SKILL_CANT_LEARN, sizeof(sk->skill));
    memset(sk->language, 0, sizeof(sk->language));
    
    /* Racial skills */
    sk->skill[SKILL_Hide] = racialSkills[raceIndex][RACIAL_HIDE_INDEX];
    sk->skill[SKILL_Sneak] = racialSkills[raceIndex][RACIAL_SNEAK_INDEX];
    sk->skill[SKILL_Forage] = racialSkills[raceIndex][RACIAL_FORAGE_INDEX];
    sk->skill[SKILL_Swimming] = racialSkills[raceIndex][RACIAL_SWIMMING_INDEX];
    sk->skill[SKILL_Tinkering] = racialSkills[raceIndex][RACIAL_TINKERING_INDEX];
    
    /* Class level 1 skills */
    for (i = 0; i < SKILL_COUNT; i++)
    {
        /* Write over the default value of any skill that hasn't been set by the racial skill values above */
        if (sk->skill[i] == SKILL_CANT_LEARN)
            sk->skill[i] = classLvl1Skills[i][classId];
    }
    
    /* Languages */
    sk->language[LANG_CommonTongue] = LANG_CAP;
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
    
    if (value > SKILL_CAP)
        value = SKILL_CAP;
    
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

int skill_get(Skills* sk, int skillId)
{
    int val;
    
    if (skillId >= SKILL_COUNT)
        skillId = SKILL_COUNT - 1;
    
    val = sk->skill[skillId];
    
    if (val > SKILL_CAP)
        val = 0;
    
    return val;
}

int skill_get_language(Skills* sk, int langId)
{
    int val;

    if (langId >= LANG_COUNT)
        langId = LANG_COUNT - 1;

    val = sk->language[langId];

    if (val > SKILL_CAP)
        val = 0;

    return val;
}
