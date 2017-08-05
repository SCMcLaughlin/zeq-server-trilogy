
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

#endif/*MISC_STRUCT_H*/
