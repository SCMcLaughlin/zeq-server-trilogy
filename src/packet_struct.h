
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

#pragma pack()

#endif/*PACKET_STRUCT_H*/
