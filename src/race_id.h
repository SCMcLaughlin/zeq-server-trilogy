
#ifndef RACE_ID_H
#define RACE_ID_H

/*fixme: add non-player races*/
enum RaceId
{
    RACE_ID_NONE,
    RACE_ID_Human = 1,
    RACE_ID_Barbarian,
    RACE_ID_Erudite,
    RACE_ID_WoodElf,
    RACE_ID_HighElf,
    RACE_ID_DarkElf,
    RACE_ID_HalfElf,
    RACE_ID_Dwarf,
    RACE_ID_Troll,
    RACE_ID_Ogre,
    RACE_ID_Halfling,
    RACE_ID_Gnome,
    RACE_ID_Iksar = 124
};

#define RACE_BIT_ALL 0x1fff

const char* race_id_to_str(int raceId);

#endif/*RACE_ID_H*/
