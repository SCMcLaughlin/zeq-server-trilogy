
#ifndef SPELL_TARGET_ID_H
#define SPELL_TARGET_ID_H

enum SpellTargetId
{
    SPELL_TARGET_LineOfSight                = 1,
    SPELL_TARGET_GroupPulse                 = 3,
    SPELL_TARGET_AreaAroundSelf             = 4,
    SPELL_TARGET_Single                     = 5,
    SPELL_TARGET_Self                       = 6,
    SPELL_TARGET_AreaAroundTarget           = 8,
    SPELL_TARGET_AnimalOnly                 = 9,
    SPELL_TARGET_UndeadOnly                 = 10,
    SPELL_TARGET_SummonedOnly               = 11,
    SPELL_TARGET_Lifetap                    = 13,
    SPELL_TARGET_Pet                        = 14,
    SPELL_TARGET_Corpse                     = 15,
    SPELL_TARGET_PlantOnly                  = 16,
    SPELL_TARGET_GiantOnly                  = 17,
    SPELL_TARGET_DragonOnly                 = 18,
    SPELL_TARGET_AreaAroundUndead           = 24,
    SPELL_TARGET_AreaAroundSelfBeneficial   = 40,
    SPELL_TARGET_Group                      = 41,
};

#endif/*SPELL_TARGET_ID_H*/
