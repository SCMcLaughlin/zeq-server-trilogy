
#ifndef CHAR_SELECT_DATA_H
#define CHAR_SELECT_DATA_H

#include "define.h"
#include "char_select_packet_struct.h"

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

#endif/*CHAR_SELECT_DATA_H*/
