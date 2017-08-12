
#ifndef ITEM_H
#define ITEM_H

#include "define.h"
#include "item_proto.h"

typedef struct {
    ItemProto*  proto;
    uint8_t     charges;
    uint8_t     stackAmt;
    bool        isBag;
} Item;

#endif/*ITEM_H*/
