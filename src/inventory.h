
#ifndef INVENTORY_H
#define INVENTORY_H

#include "define.h"
#include "aligned.h"
#include "client_load_data.h"
#include "item.h"
#include "item_proto.h"
#include "misc_struct.h"
#include "ringbuf.h"

struct Client;

typedef struct {
    uint32_t    realId;
    uint16_t    clientId;
} ItemIdMap;

typedef struct {
    uint16_t    slotId;
    uint32_t    itemId;
    Item*       item;
} InvSlot;

typedef struct {
    int16_t     primaryIndex;
    int16_t     secondaryIndex;
    int16_t     rangeIndex;
    int16_t     ammoIndex;
    uint16_t    slotCount;
    uint16_t    cursorCount;
    uint16_t    idMapCount;
    uint16_t    loreItemIdCount;
    InvSlot     cursor;
    InvSlot*    slots;
    InvSlot*    cursorQueue;
    ItemIdMap*  idMap;
    uint32_t*   loreItemIds;
} Inventory;

void inv_init(Inventory* inv);
void inv_deinit(Inventory* inv);

int inv_from_db(Inventory* inv, ClientLoadData_Inventory* data, uint32_t count, ItemList* itemList);

uint16_t inv_item_id_for_client(Inventory* inv, uint32_t itemIdReal);

void inv_send_all(Inventory* inv, struct Client* client, RingBuf* udpQueue);

void inv_calc_stats(Inventory* inv, CoreStats* stats, uint32_t* weight);
void inv_calc_held_weight(Inventory* inv, uint32_t* weight);

void inv_write_pp_main_item_ids(Inventory* inv, Aligned* a);
void inv_write_pp_main_item_properties(Inventory* inv, Aligned* a);
void inv_write_pp_main_bag_item_ids(Inventory* inv, Aligned* a);
void inv_write_pp_main_bag_item_properties(Inventory* inv, Aligned* a);
void inv_write_pp_bank_item_ids(Inventory* inv, Aligned* a);
void inv_write_pp_bank_item_properties(Inventory* inv, Aligned* a);
void inv_write_pp_bank_bag_item_ids(Inventory* inv, Aligned* a);
void inv_write_pp_bank_bag_item_properties(Inventory* inv, Aligned* a);

enum InventorySlot
{
    INV_SLOT_NONE,
    INV_SLOT_Ear1,
    INV_SLOT_Head,
    INV_SLOT_Face,
    INV_SLOT_Ear2,
    INV_SLOT_Neck,
    INV_SLOT_Shoulders,
    INV_SLOT_Arms,
    INV_SLOT_Back,
    INV_SLOT_Wrist1,
    INV_SLOT_Wrist2,
    INV_SLOT_Range,
    INV_SLOT_Hands,
    INV_SLOT_Primary,
    INV_SLOT_Secondary,
    INV_SLOT_Fingers1,
    INV_SLOT_Fingers2,
    INV_SLOT_Chest,
    INV_SLOT_Legs,
    INV_SLOT_Feet,
    INV_SLOT_Waist,
    INV_SLOT_Ammo,
    INV_SLOT_MainInventory0,
    INV_SLOT_MainInventory1,
    INV_SLOT_MainInventory2,
    INV_SLOT_MainInventory3,
    INV_SLOT_MainInventory4,
    INV_SLOT_MainInventory5,
    INV_SLOT_MainInventory6,
    INV_SLOT_MainInventory7,
    INV_SLOT_Cursor,
    
    INV_SLOT_EquipForStatsBegin             = INV_SLOT_Ear1,
    INV_SLOT_EquipForStatsEnd               = INV_SLOT_Waist,
    INV_SLOT_EquipMainAndCursorBegin        = INV_SLOT_Ear1,
    INV_SLOT_EquipMainAndCursorEnd          = INV_SLOT_Cursor,
    INV_SLOT_MainInventoryBegin             = INV_SLOT_MainInventory0,
    INV_SLOT_MainInventoryEnd               = INV_SLOT_MainInventory7,
    INV_SLOT_BagSlotsBegin                  = 251,
    INV_SLOT_BagSlotsEnd                    = 330,
    INV_SLOT_BagsSlotsIncludingCursorBegin  = 251,
    INV_SLOT_BagsSlotsIncludingCursorEnd    = 340,
    INV_SLOT_BankBegin                      = 2000,
    INV_SLOT_BankEnd                        = 2007,
    INV_SLOT_BankExtendedBegin              = INV_SLOT_BankBegin,
    INV_SLOT_BankExtendedEnd                = 2015,
    INV_SLOT_BankBagSlotsBegin              = 2031,
    INV_SLOT_BankBagSlotsEnd                = 2110,
    INV_SLOT_BankExtendedBagSlotsBegin      = INV_SLOT_BankBagSlotsBegin,
    INV_SLOT_BankExtendedBagSlotsEnd        = 2190,
};

#endif/*INVENTORY_H*/
