
#ifndef PLAYER_PROFILE_PACKET_STRUCT_H
#define PLAYER_PROFILE_PACKET_STRUCT_H

#include "define.h"

#define ZEQ_PP_MAX_BUFFS 15
#define ZEQ_PP_SKILLS_COUNT 74
#define ZEQ_PP_LANGUAGES_COUNT 24
#define ZEQ_PP_SPELLBOOK_SLOT_COUNT 256
#define ZEQ_PP_MEMMED_SPELL_SLOT_COUNT 8
#define ZEQ_PP_MAX_NAME_LENGTH 30
#define ZEQ_PP_MAX_SURNAME_LENGTH 20
#define ZEQ_PP_MAX_ZONE_SHORT_NAME_LENGTH 32
#define ZEQ_PP_MAX_BIND_POINT_ZONE_SHORT_NAME_LENGTH 20
#define ZEQ_PP_BIND_POINT_COUNT 5
#define ZEQ_PP_MAIN_INVENTORY_SLOT_COUNT 30
#define ZEQ_PP_BAGGED_INVENTORY_SLOT_COUNT 90
#define ZEQ_PP_BANK_INVENTORY_SLOT_COUNT 8
#define ZEQ_PP_BANK_BAGGED_INVENTORY_SLOT_COUNT 80
#define ZEQ_PP_GROUP_MEMBER_COUNT 5

#pragma pack(1)

typedef struct {
    uint8_t     visibility; /* 0 = not visible, 1 = permanent, 2 = normal */
    uint8_t     casterLevel;
    uint8_t     bardModifier;
    uint8_t     unknown;
    uint16_t    spellId;
    uint32_t    tickDuration;
} PS_Buff;

typedef struct {
    uint32_t    pp;
    uint32_t    gp;
    uint32_t    sp;
    uint32_t    cp;
} PS_Coin;

typedef struct {
    uint16_t    unknownA;
    uint8_t     charges;    /* 0xff = unlimited */
    uint8_t     unknownB;   /* maybe charges is 16bit? */
    uint16_t    unknownDefaultFF;
    uint8_t     unknownC[4];
} PS_PPItem;

typedef struct {
    char    name[16];
    uint8_t unknown[32];
} PS_PPGroupMember;

#define PP_FIELDS                                                                                                       \
    char                    name[ZEQ_PP_MAX_NAME_LENGTH];                                                               \
    char                    surname[ZEQ_PP_MAX_SURNAME_LENGTH];                                                         \
    uint16_t                genderId;                                                                                   \
    uint16_t                raceId;                                                                                     \
    uint16_t                classId;                                                                                    \
    uint32_t                level;                                                                                      \
    uint32_t                experience;                                                                                 \
    uint16_t                trainingPoints;                                                                             \
    int16_t                 currentMana;                                                                                \
    uint8_t                 face;                                                                                       \
    uint8_t                 unknownA[47];                                                                               \
    int16_t                 currentHp;                                                                                  \
    uint8_t                 unknownB;                                                                                   \
    uint8_t                 STR;                                                                                        \
    uint8_t                 STA;                                                                                        \
    uint8_t                 CHA;                                                                                        \
    uint8_t                 DEX;                                                                                        \
    uint8_t                 INT;                                                                                        \
    uint8_t                 AGI;                                                                                        \
    uint8_t                 WIS;                                                                                        \
    uint8_t                 languages[ZEQ_PP_LANGUAGES_COUNT];                                                          \
    uint8_t                 unknownC[14];                                                                               \
    uint16_t                mainInventoryItemIds[ZEQ_PP_MAIN_INVENTORY_SLOT_COUNT];                                     \
    uint32_t                mainInventoryInternalUnused[ZEQ_PP_MAIN_INVENTORY_SLOT_COUNT];                              \
    PS_PPItem               mainInventoryItemProperties[ZEQ_PP_MAIN_INVENTORY_SLOT_COUNT];                              \
    PS_Buff                 buffs[ZEQ_PP_MAX_BUFFS];                                                                    \
    uint16_t                baggedItemIds[ZEQ_PP_BAGGED_INVENTORY_SLOT_COUNT];  /* [80] to [89] are cursor bag slots */ \
    PS_PPItem               baggedItemProperties[ZEQ_PP_BAGGED_INVENTORY_SLOT_COUNT];                                   \
    uint16_t                spellbook[ZEQ_PP_SPELLBOOK_SLOT_COUNT];                                                     \
    uint16_t                memmedSpellIds[ZEQ_PP_MEMMED_SPELL_SLOT_COUNT];                                             \
    uint16_t                unknownD;                                                                                   \
    float                   y; /*fixme: confirm this*/                                                                  \
    float                   x;                                                                                          \
    float                   z;                                                                                          \
    float                   heading;                                                                                    \
    char                    zoneShortName[ZEQ_PP_MAX_ZONE_SHORT_NAME_LENGTH];                                           \
    uint32_t                unknownEDefault100;   /* Breath or something? */                                            \
    PS_Coin                 coins;                                                                                      \
    PS_Coin                 coinsBank;                                                                                  \
    PS_Coin                 coinsCursor;                                                                                \
    uint8_t                 skills[ZEQ_PP_SKILLS_COUNT];                                                                \
    uint8_t                 unknownF[162];                                                                              \
    uint32_t                autoSplit;  /* 0 or 1 */                                                                    \
    uint32_t                pvpEnabled;                                                                                 \
    uint8_t                 unknownG[12]; /* Starts with uint32_t isAfk? */                                             \
    uint32_t                isGM;                                                                                       \
    uint8_t                 unknownH[20];                                                                               \
    uint32_t                disciplinesReady;                                                                           \
    uint8_t                 unknownI[20];                                                                               \
    uint32_t                hunger;                                                                                     \
    uint32_t                thirst;                                                                                     \
    uint8_t                 unknownJ[24];                                                                               \
    char                    bindZoneShortName[ZEQ_PP_BIND_POINT_COUNT][ZEQ_PP_MAX_BIND_POINT_ZONE_SHORT_NAME_LENGTH];   \
    PS_PPItem               bankInventoryItemProperties[ZEQ_PP_BANK_INVENTORY_SLOT_COUNT];                              \
    PS_PPItem               bankBaggedItemProperties[ZEQ_PP_BANK_BAGGED_INVENTORY_SLOT_COUNT];                          \
    uint32_t                unknownK;                                                                                   \
    uint32_t                unknownL;                                                                                   \
    float                   bindLocY[ZEQ_PP_BIND_POINT_COUNT];                                                          \
    float                   bindLocX[ZEQ_PP_BIND_POINT_COUNT];                                                          \
    float                   bindLocZ[ZEQ_PP_BIND_POINT_COUNT];                                                          \
    float                   bindLocHeading[ZEQ_PP_BIND_POINT_COUNT];                                                    \
    uint32_t                bankInventoryInternalUnused[ZEQ_PP_BANK_INVENTORY_SLOT_COUNT];                              \
    uint8_t                 unknownM[12];                                                                               \
    uint32_t                unixTimeA;  /* Creation time? */                                                            \
    uint8_t                 unknownN[8];                                                                                \
    uint32_t                unknownODefault1;                                                                           \
    uint8_t                 unknownP[8];                                                                                \
    uint16_t                bankInventoryItemIds[ZEQ_PP_BANK_INVENTORY_SLOT_COUNT];                                     \
    uint16_t                bankBaggedItemIds[ZEQ_PP_BANK_BAGGED_INVENTORY_SLOT_COUNT];                                 \
    uint16_t                deity;                                                                                      \
    uint16_t                guildId;                                                                                    \
    uint32_t                unixTimeB;                                                                                  \
    uint8_t                 unknownQ[4];                                                                                \
    uint16_t                unknownRDefault7f7f;                                                                        \
    uint8_t                 fatiguePercent; /* Percentage taken out of the yellow bar, i.e. (100 - fatiguePercent) */   \
    uint8_t                 unknownS;                                                                                   \
    uint8_t                 unknownTDefault1;                                                                           \
    uint16_t                anon;                                                                                       \
    uint8_t                 guildRank;                                                                                  \
    uint8_t                 drunkeness;                                                                                 \
    uint8_t                 showEqLoadScreen;                                                                           \
    uint16_t                unknownU;                                                                                   \
    uint32_t                spellGemRefreshMilliseconds[ZEQ_PP_MEMMED_SPELL_SLOT_COUNT];                                \
    uint32_t                unknownV; /* Is the above field supposed to be 4 bytes lower? */                            \
    uint32_t                harmtouchRefreshMilliseconds;                                                               \
    PS_PPGroupMember        groupMember[ZEQ_PP_GROUP_MEMBER_COUNT];                                                     \
    uint8_t                 unknownW[3644]
    
typedef struct {
    uint32_t    crc;    /* crc_calc32, negated */
    PP_FIELDS;
} PS_PlayerProfile;

/* Used during character creation */
typedef struct {
    PP_FIELDS;
} PSCS_PlayerProfile;

#pragma pack()

#undef PP_FIELDS

#endif/*PLAYER_PROFILE_PACKET_STRUCT_H*/
