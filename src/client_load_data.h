
#ifndef CLIENT_LOAD_DATA_H
#define CLIENT_LOAD_DATA_H

#include "define.h"
#include "buffer.h"
#include "loc.h"
#include "misc_struct.h"
#include "util_atomic.h"

typedef struct {
    int64_t         charId;
    StaticBuffer*   surname;
    uint8_t         level;
    uint8_t         classId;
    uint16_t        raceId;
    uint8_t         genderId;
    uint8_t         faceId;
    uint16_t        deityId;
    LocH            loc;
    int64_t         currentHp;
    int64_t         currentMana;
    int64_t         currentEndurance;
    int64_t         experience;
    uint16_t        trainingPoints;
    uint8_t         STR;
    uint8_t         STA;
    uint8_t         DEX;
    uint8_t         AGI;
    uint8_t         INT;
    uint8_t         WIS;
    uint8_t         CHA;
    uint8_t         guildRank;
    uint32_t        guildId;
    uint64_t        harmtouchTimestamp;
    uint64_t        disciplineTimestamp;
    Coin            coin;
    Coin            coinCursor;
    Coin            coinBank;
    uint16_t        hunger;
    uint16_t        thirst;
    bool            isGM;
    uint8_t         anon;
    uint16_t        drunkeness;
    uint64_t        creationTimestamp;
    int             zoneId;
    int             instId;
} ClientLoadData_Character;

#endif/*CLIENT_LOAD_DATA_H*/
