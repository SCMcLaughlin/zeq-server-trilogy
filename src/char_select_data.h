
#ifndef CHAR_SELECT_DATA_H
#define CHAR_SELECT_DATA_H

#include "define.h"
#include "char_select_packet_struct.h"
#include "player_profile_packet_struct.h"

typedef struct {
    StaticBuffer*   name[CHAR_SELECT_MAX_CHARS];
    int             zoneId[CHAR_SELECT_MAX_CHARS];
    uint8_t         level[CHAR_SELECT_MAX_CHARS];
    uint8_t         classId[CHAR_SELECT_MAX_CHARS];
    int16_t         raceId[CHAR_SELECT_MAX_CHARS];
    uint8_t         genderId[CHAR_SELECT_MAX_CHARS];
    uint8_t         faceId[CHAR_SELECT_MAX_CHARS];
    uint8_t         materialIds[CHAR_SELECT_MAX_CHARS][CHAR_SELECT_MATERIAL_COUNT];
    uint32_t        materialTints[CHAR_SELECT_MAX_CHARS][CHAR_SELECT_MATERIAL_COUNT];
} CharSelectData;

typedef struct {
    char        name[ZEQ_PP_MAX_NAME_LENGTH];
    uint8_t     genderId;
    uint8_t     classId;
    uint8_t     faceId;
    uint8_t     currentHp;
    int16_t     raceId;
    union {
        struct {
            uint8_t STR;
            uint8_t STA;
            uint8_t CHA;
            uint8_t DEX;
            uint8_t INT;
            uint8_t AGI;
            uint8_t WIS;
        };
        uint8_t stats[7];
    };
    int         zoneId;
    uint16_t    deityId;
    /* Note: the client should not be trusted to provide any of the below at all */
    float       x, y, z, heading;
    struct {
        int     zoneId;
        float   x, y, z;
    } bindPoint[5];
} CharCreateData;

#endif/*CHAR_SELECT_DATA_H*/
