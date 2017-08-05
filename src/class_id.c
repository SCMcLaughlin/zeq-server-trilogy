
#include "class_id.h"

const char* class_id_to_str(int classId)
{
    const char* ret;
    switch (classId)
    {
    default:
    case CLASS_ID_Warrior: ret = "Warrior"; break;
    case CLASS_ID_Cleric: ret = "Cleric"; break;
    case CLASS_ID_Paladin: ret = "Paladin"; break;
    case CLASS_ID_Ranger: ret = "Ranger"; break;
    case CLASS_ID_ShadowKnight: ret = "Shadow Knight"; break;
    case CLASS_ID_Druid: ret = "Druid"; break;
    case CLASS_ID_Monk: ret = "Monk"; break;
    case CLASS_ID_Bard: ret = "Bard"; break;
    case CLASS_ID_Rogue: ret = "Rogue"; break;
    case CLASS_ID_Shaman: ret = "Shaman"; break;
    case CLASS_ID_Necromancer: ret = "Necromancer"; break;
    case CLASS_ID_Wizard: ret = "Wizard"; break;
    case CLASS_ID_Magician: ret = "Magician"; break;
    case CLASS_ID_Enchanter: ret = "Enchanter"; break;
    }
    return ret;
}
