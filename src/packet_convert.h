
#ifndef PACKET_CONVERT_H
#define PACKET_CONVERT_H

#include "define.h"
#include "buffer.h"

typedef struct {
    StaticBuffer*   name;
    StaticBuffer*   lore;
    uint8_t         itemType;
    uint8_t         model;
    uint8_t         weight;
    bool            isPermanent;
    bool            isDroppable;
    bool            isUnique;
    bool            isMagic;
    uint16_t        itemId;
    uint16_t        icon;
    uint16_t        currentSlot;
    uint32_t        slots;
    uint32_t        cost;
    int8_t          STR;
    int8_t          STA;
    int8_t          CHA;
    int8_t          DEX;
    int8_t          INT;
    int8_t          AGI;
    int8_t          WIS;
    int8_t          MR;
    int8_t          FR;
    int8_t          CR;
    int8_t          DR;
    int8_t          PR;
    int8_t          hp;
    int8_t          mana;
    int8_t          AC;
    bool            isStackable;
    bool            isUnlimitedCharges;
    uint8_t         light;
    uint8_t         delay;
    uint8_t         damage;
    uint8_t         clickyType; /* 0 = none/proc, 1 = unrestricted clicky, 2 = worn, 3 = unrestricted expendable, 4 = must-equip clicky, 5 = class-restricted clicky */
    uint8_t         range;
    uint8_t         skill;
    uint8_t         clickableLevel;
    uint8_t         materialId;
    uint8_t         consumableType;
    uint8_t         procLevel;
    uint8_t         hastePercent;
    uint8_t         charges;
    uint8_t         effectType; /* 0 = none/proc, 1 = unrestricted clicky, 2 = worn, 3 = unrestricted expendable, 4 = must-equip clicky, 5 = class-restricted clicky */
    uint16_t        clickySpellId;
    uint16_t        spellId;
    uint32_t        tint;
    uint32_t        classes;
    uint32_t        races;
    uint32_t        deities;
    uint32_t        castingTime;
    struct {
        uint8_t     type;
        uint8_t     capacity;
        uint8_t     isOpen;
        uint8_t     containableSize;
        uint8_t     weightReductionPercent;
    } bag;
} PC_Item;

void pc_item_set_defaults(PC_Item* item);

#endif/*PACKET_CONVERT_H*/
