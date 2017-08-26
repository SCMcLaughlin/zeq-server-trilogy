
#ifndef MISC_ENUM_H
#define MISC_ENUM_H

enum SpawnAppearanceType
{
    SPAWN_APPEARANCE_SendToBind     = 0,
    SPAWN_APPEARANCE_LevelChange    = 1,
    SPAWN_APPEARANCE_Invisible      = 3,
    SPAWN_APPEARANCE_PvP            = 4,
    SPAWN_APPEARANCE_Light          = 5,
    SPAWN_APPEARANCE_UprightState   = 14,
    SPAWN_APPEARANCE_Sneaking       = 15,
    SPAWN_APPEARANCE_SetEntityId    = 16,
    SPAWN_APPEARANCE_HpRegenTick    = 17,   /* Sent by the client to tell us when to regen. Very much ignored by us. */
    SPAWN_APPEARANCE_Linkdead       = 18,
    SPAWN_APPEARANCE_FlyMode        = 19,   /* 0 = none, 1 = flymode, 2 = levitate */
    SPAWN_APPEARANCE_GMFlag         = 20,
    SPAWN_APPEARANCE_AnonSetting    = 21,
    SPAWN_APPEARANCE_GuildId        = 22,
    SPAWN_APPEARANCE_GuildRank      = 23,
    SPAWN_APPEARANCE_Afk            = 24,
    SPAWN_APPEARANCE_AutoSplit      = 28,
    SPAWN_APPEARANCE_Size           = 29,
    SPAWN_APPEARANCE_BecomeNpc      = 30,
};

enum UprightStateType
{
    UPRIGHT_STATE_Standing      = 100,
    UPRIGHT_STATE_LoseControl   = 102,
    UPRIGHT_STATE_Looting       = 105,
    UPRIGHT_STATE_Sitting       = 110,
    UPRIGHT_STATE_Ducking       = 111,
    UPRIGHT_STATE_Unconscious   = 115,
};

enum BodyType
{
    BODY_TYPE_NONE              = 0,
    BODY_TYPE_Humanoid          = 1,
    BODY_TYPE_Lycanthrope       = 2,
    BODY_TYPE_Undead            = 3,
    BODY_TYPE_Giant             = 4,
    BODY_TYPE_Construct         = 5,
    BODY_TYPE_ExtraPlanar       = 6,
    BODY_TYPE_Magical           = 7,
    BODY_TYPE_SummonedUndead    = 8,
    BODY_TYPE_Untargetable      = 11,
    BODY_TYPE_Vampire           = 12,
    BODY_TYPE_Animal            = 21,
    BODY_TYPE_Insect            = 22,
    BODY_TYPE_Monster           = 23,
    BODY_TYPE_Summoned          = 24,
    BODY_TYPE_Plant             = 25,
    BODY_TYPE_Dragon            = 26,
    BODY_TYPE_Hidden            = 65,
    BODY_TYPE_InvisibleMan      = 66,
};

enum FlyModeType
{
    FLY_MODE_Grounded,
    FLY_MODE_Flying,
    FLY_MODE_Levitating,
    FLY_MODE_Water,
};

enum BuffPacketType
{
    BUFF_PACKET_TYPE_Remove = 1,
    BUFF_PACKET_TYPE_Add = 2,
    BUFF_PACKET_TYPE_Update = 3,
};

#endif/*MISC_ENUM_H*/
