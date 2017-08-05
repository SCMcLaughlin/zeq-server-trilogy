
#include "race_id.h"

const char* race_id_to_str(int raceId)
{
    const char* ret;
    switch (raceId)
    {
    default:
    case RACE_ID_Human: ret = "Human"; break;
    case RACE_ID_Barbarian: ret = "Barbarian"; break;
    case RACE_ID_Erudite: ret = "Erudite"; break;
    case RACE_ID_WoodElf: ret = "Wood Elf"; break;
    case RACE_ID_HighElf: ret = "High Elf"; break;
    case RACE_ID_DarkElf: ret = "Dark Elf"; break;
    case RACE_ID_HalfElf: ret = "Half Elf"; break;
    case RACE_ID_Dwarf: ret = "Dwarf"; break;
    case RACE_ID_Troll: ret = "Troll"; break;
    case RACE_ID_Ogre: ret = "Ogre"; break;
    case RACE_ID_Halfling: ret = "Halfling"; break;
    case RACE_ID_Gnome: ret = "Gnome"; break;
    case RACE_ID_Iksar: ret = "Iksar"; break;
    }
    return ret;
}
