
#ifndef CLASS_ID_H
#define CLASS_ID_H

/*fixme: add non-player classes*/
enum ClassId
{
    CLASS_ID_NONE,
    CLASS_ID_Warrior,
    CLASS_ID_Cleric,
    CLASS_ID_Paladin,
    CLASS_ID_Ranger,
    CLASS_ID_ShadowKnight,
    CLASS_ID_Druid,
    CLASS_ID_Monk,
    CLASS_ID_Bard,
    CLASS_ID_Rogue,
    CLASS_ID_Shaman,
    CLASS_ID_Necromancer,
    CLASS_ID_Wizard,
    CLASS_ID_Magician,
    CLASS_ID_Enchanter,
};

const char* class_id_to_str(int classId);

#endif/*CLASS_ID_H*/
