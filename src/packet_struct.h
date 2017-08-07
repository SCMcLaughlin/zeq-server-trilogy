
#ifndef PACKET_STRUCT_H
#define PACKET_STRUCT_H

#include "define.h"

#pragma pack(1)

typedef struct {
    uint32_t    checksum;
    char        name[32];
} PS_ZoneEntryClient;

typedef struct {
    uint32_t    crc;
    uint8_t     unknownA;
    char        name[30];
    char        zoneShortName[20];
    uint8_t     unknownB;
    float       y;
    float       x;
    float       z;
    float       heading;
    uint8_t     unknownC[76];
    uint16_t    guildId;
    uint8_t     unknownD[7];    /* [6] = 2 makes the client instantly die upon zoning in */
    uint8_t     classId;
    uint16_t    raceId;
    uint8_t     genderId;
    uint8_t     level;
    uint8_t     unknownE[2];
    uint8_t     isPvP;
    uint8_t     unknownF[2];
    uint8_t     faceId;
    uint8_t     helmTextureId;
    uint8_t     unknownG[6];
    uint8_t     secondaryWeaponModelId;
    uint8_t     primaryWeaponModelId;
    uint8_t     unknownH[3];
    uint32_t    helmColor;
    uint8_t     unknownI[32];
    uint8_t     textureId;      /* 0xff for standard client gear */
    uint8_t     unknownJ[19];
    float       walkingSpeed;
    float       runningSpeed;
    uint8_t     unknownK[12];
    uint8_t     anon;
    uint8_t     unknownL[23];
    char        surname[20];
    uint8_t     unknownM[2];
    uint16_t    deityId;
    uint8_t     unknownN[8];
} PS_ZoneEntry;

typedef struct {
    int type;
    int intensity;
} PS_Weather;

typedef struct {
    char        characterName[30];
    char        zoneShortName[20];
    char        zoneLongName[180];
    uint8_t     zoneType;               /* Default 0xff */
    uint8_t     fogRed[4];
    uint8_t     fogGreen[4];
    uint8_t     fogBlue[4];
    uint8_t     unknownA;
    float       fogMinClippingDistance[4];
    float       fogMaxClippingDistance[4];
    float       gravity;                /* Default 0.4f */
    uint8_t     unknownB[50];
    uint16_t    skyType;
    uint8_t     unknownC[8];
    float       unknownD;               /* Default 0.75 */
    float       safeSpotX;
    float       safeSpotY;
    float       safeSpotZ;
    float       safeSpotHeading;        /* Assumed */
    float       minZ;                   /* If a client falls below this, they are sent to the safe spot; default -32000.0f */
    float       minClippingDistance;    /* Default 1000.0f */
    float       maxClippingDistance;    /* Default 20000.0f */
    uint8_t     unknownE[32];
} PS_ZoneInfo;

typedef struct {
    int16_t     entityId;
    uint16_t    unknownA;
    int16_t     typeId;
    uint16_t    unknownB;
    int         value;
} PS_SpawnAppearance;

typedef struct {
    uint32_t    unknownA;
    float       size;
    float       walkingSpeed;
    float       runningSpeed;
    uint32_t    tints[7];
    uint8_t     unknownB[9];    /* Equip-related ? */
    int8_t      heading;
    int8_t      headingDelta;
    int16_t     y;
    int16_t     x;
    int16_t     z;
    int         yDelta  : 10;
    int         unusedA : 1;
    int         zDelta  : 10;
    int         unusedB : 1;
    int         xDelta  : 10;
    uint8_t     unknownC;
    uint16_t    entityId;
    uint16_t    bodyType;
    uint16_t    ownerEntityId;  /* 0 = not a pet */
    int16_t     hpPercent;
    uint16_t    guildId;
    uint8_t     raceId;
    uint8_t     mobType;        /* 0 = client, 1 = npc, 2 = client corpse, 3 = npc corpse, 10 = self */
    uint8_t     classId;
    uint8_t     genderId;
    uint8_t     level;
    uint8_t     isInvisible;
    uint8_t     unknownD;
    uint8_t     isPvP;
    uint8_t     uprightState;   /* UPRIGHT_STATE_Standing and such */
    uint8_t     lightLevel;
    uint8_t     anon;
    uint8_t     isAfk;
    uint8_t     unknownE;
    uint8_t     isLinkdead;
    uint8_t     isGM;
    uint8_t     unknownF;
    uint8_t     textureId;
    uint8_t     helmTextureId;
    uint8_t     unknownG;
    uint8_t     materialIds[9];
    char        name[30];
    char        surname[20];
    uint8_t     guildRank;
    uint8_t     unknownH;
    uint16_t    deityId;  /*fixme: may be uint8?*/
    uint8_t     unknownI[10];
} PS_Spawn;

#pragma pack()

#endif/*PACKET_STRUCT_H*/
