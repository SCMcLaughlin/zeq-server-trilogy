
#ifndef ITEM_H
#define ITEM_H

#include "define.h"

typedef struct {
    /*ItemProto*  proto;*/
    uint16_t    charges;
    uint16_t    stackAmt;
    bool        isBag;
} Item;

#endif/*ITEM_H*/
