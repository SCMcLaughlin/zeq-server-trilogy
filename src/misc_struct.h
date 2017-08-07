
#ifndef MISC_STRUCT_H
#define MISC_STRUCT_H

#include "define.h"

typedef struct {
    int64_t     maxHp;
    int64_t     maxMana;
    int64_t     maxEndurance;
    int         STR;
    int         STA;
    int         DEX;
    int         AGI;
    int         INT;
    int         WIS;
    int         CHA;
    int         svMagic;
    int         svFire;
    int         svCold;
    int         svPoison;
    int         svDisease;
} CoreStats;

typedef struct {
    uint32_t    pp, gp, sp, cp;
} Coin;

typedef struct {
    uint8_t     red;
    uint8_t     green;
    uint8_t     blue;
    float       minClippingDistance;
    float       maxClippingDistance;
} ZoneFog;

#endif/*MISC_STRUCT_H*/
