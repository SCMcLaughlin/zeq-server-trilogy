
#ifndef CHAR_SELECT_PACKET_STRUCT_H
#define CHAR_SELECT_PACKET_STRUCT_H

#include "define.h"

#define CHAR_SELECT_MAX_CHARS 10
#define CHAR_SELECT_MAX_NAME_LENGTH 30
#define CHAR_SELECT_MAX_ZONE_SHORTNAME_LENGTH 20
#define CHAR_SELECT_MATERIAL_COUNT 9
#define CHAR_SELECT_MAX_GUILD_NAME_LENGTH 32
#define CHAR_SELECT_MAX_GUILD_COUNT 512
#define CHAR_SELECT_MAX_NAME_APPROVAL_LENGTH 32
#define CHAR_SELECT_MAX_ZONE_IP_LENGTH 75
#define CHAR_SELECT_MAX_ZONE_ADDRESS_SHORTNAME_LENGTH 53

#pragma pack(1)

typedef struct {
    char        names[CHAR_SELECT_MAX_CHARS][CHAR_SELECT_MAX_NAME_LENGTH];
    uint8_t     levels[CHAR_SELECT_MAX_CHARS];
    uint8_t     classIds[CHAR_SELECT_MAX_CHARS];
    int16_t     raceIds[CHAR_SELECT_MAX_CHARS];
    char        zoneShortNames[CHAR_SELECT_MAX_CHARS][CHAR_SELECT_MAX_ZONE_SHORTNAME_LENGTH];
    uint8_t     genderIds[CHAR_SELECT_MAX_CHARS];
    uint8_t     faceIds[CHAR_SELECT_MAX_CHARS];
    uint8_t     materialIds[CHAR_SELECT_MAX_CHARS][CHAR_SELECT_MATERIAL_COUNT];
    uint8_t     unknownA[2];
    uint32_t    materialTints[CHAR_SELECT_MAX_CHARS][CHAR_SELECT_MATERIAL_COUNT];
    uint8_t     unknownB[20];
    uint32_t    primaryWeaponIds[CHAR_SELECT_MAX_CHARS];
    uint32_t    secondaryWeaponIds[CHAR_SELECT_MAX_CHARS];
    uint8_t     unknownC[148];
} PSCS_CharacterInfo;

typedef struct {
    uint32_t    guildId;    /* If does not exist, use 0xffffffff */
    char        name[CHAR_SELECT_MAX_GUILD_NAME_LENGTH];
    uint32_t    unknownA;   /* 0xffffffff */
    uint32_t    exists;
    uint32_t    unknownB;
    uint32_t    unknownC;   /* 0xffffffff */
    uint32_t    unknownD;
} PSCS_Guild;

typedef struct {
    uint32_t    unknown;    /* Count? */
    PSCS_Guild  guilds[CHAR_SELECT_MAX_GUILD_COUNT];
} PSCS_GuildList;

typedef struct {
    char        name[CHAR_SELECT_MAX_NAME_APPROVAL_LENGTH];
    uint32_t    raceId;
    uint32_t    classId;
} PSCS_NameApproval;

typedef struct {
    uint32_t    unusedSpawnId;
    uint8_t     slotId;
    uint8_t     materialId; /* Also used for IT# - Velious weapon models only go up to 250 */
    uint16_t    operationId;
    uint32_t    color;
    uint8_t     unknownA;
    uint8_t     flag;
    uint16_t    unknownB;
} PSCS_WearChange;

typedef struct {
    char    shortName[CHAR_SELECT_MAX_ZONE_SHORTNAME_LENGTH];
} PSCS_ZoneUnavailable;

typedef struct {
    char        ipAddress[CHAR_SELECT_MAX_ZONE_IP_LENGTH];
    char        zoneShortName[CHAR_SELECT_MAX_ZONE_ADDRESS_SHORTNAME_LENGTH];
    uint16_t    port;
} PSCS_ZoneAddress;

typedef struct {
    uint8_t     hour;
    uint8_t     minute;
    uint8_t     day;
    uint8_t     month;
    uint16_t    year;
} PSCS_TimeOfDay;

#pragma pack()

#endif/*CHAR_SELECT_PACKET_STRUCT_H*/
