
#include "inventory.h"
#include "aligned.h"
#include "bit.h"
#include "player_profile_packet_struct.h"
#include "util_alloc.h"

#define INV_CLIENT_ITEM_ID_OFFSET 100

void inv_init(Inventory* inv)
{
    inv->primaryIndex = -1;
    inv->secondaryIndex = -1;
    inv->rangeIndex = -1;
    inv->ammoIndex = -1;
    inv->slotCount = 0;
    inv->cursorCount = 0;
    inv->idMapCount = 0;
    inv->slots = NULL;
    inv->cursorQueue = NULL;
    inv->idMap = NULL;
}

static InvSlot* inv_free_slots(InvSlot* slots, uint32_t n)
{
    if (slots)
    {
        uint32_t i;
        
        for (i = 0; i < n; i++)
        {
            InvSlot* slot = &slots[i];
            
            free_if_exists(slot->item);
        }
        
        free(slots);
    }
    
    return NULL;
}

void inv_deinit(Inventory* inv)
{
    inv->slots = inv_free_slots(inv->slots, inv->slotCount);
    inv->cursorQueue = inv_free_slots(inv->cursorQueue, inv->cursorCount);
    free_if_exists(inv->idMap);
}

uint16_t inv_item_id_for_client(Inventory* inv, uint32_t itemIdReal)
{
    ItemIdMap* map = inv->idMap;
    uint16_t n = inv->idMapCount;
    uint16_t i;
    
    for (i = 0; i < n; i++)
    {
        if (map[i].realId == itemIdReal)
            return map[i].clientId;
    }
    
    if (bit_is_pow2_or_zero(n))
    {
        uint32_t cap = (n == 0) ? 1 : n * 2;
        ItemIdMap* idMap = realloc_array_type(inv->idMap, cap, ItemIdMap);
        
        if (!idMap) return 0;
        
        inv->idMap = idMap;
    }
    
    i = n + INV_CLIENT_ITEM_ID_OFFSET;
    inv->idMap[n].realId = itemIdReal;
    inv->idMap[n].clientId = i;
    inv->idMapCount = n + 1;
    return i;
}

static void inv_write_pp_item_ids(Inventory* inv, Aligned* a, uint32_t from, uint32_t to)
{
    uint32_t diff = from - to;
    uint32_t resetTo = aligned_position(a);
    InvSlot* slots;
    uint32_t n;
    uint32_t i;
    
    /* Pre-fill all the spaces */
    aligned_write_memset_no_advance(a, 0xff, diff * sizeof(uint16_t));
    
    slots = inv->slots;
    n = inv->slotCount;
    
    for (i = 0; i < n; i++)
    {
        InvSlot* slot = &slots[i];
        uint16_t slotId = slot->slotId;
        
        if (slotId < from || slotId > to)
            continue;
        
        slotId -= from;
        
        aligned_advance_by(a, sizeof(uint16_t) * slotId);
        aligned_write_uint16(a, inv_item_id_for_client(inv, slot->itemId));
        aligned_reset_cursor_to(a, resetTo);
    }
    
    aligned_advance_by(a, diff * sizeof(uint16_t));
}

static void inv_write_pp_item_properties(Inventory* inv, Aligned* a, uint32_t from, uint32_t to)
{
    uint32_t diff = from - to;
    uint32_t resetTo = aligned_position(a);
    InvSlot* slots;
    uint32_t n;
    uint32_t i;
    
    /* Pre-fill all the spaces */
    for (i = 0; i < diff; i++)
    {
        aligned_write_uint32(a, 0);
        aligned_write_uint16(a, 0xffff);
        aligned_write_uint32(a, 0);
    }
    
    aligned_reset_cursor_to(a, resetTo);
    
    slots = inv->slots;
    n = inv->slotCount;
    
    for (i = 0; i < n; i++)
    {
        InvSlot* slot = &slots[i];
        uint16_t slotId = slot->slotId;
        
        if (slotId < from || slotId > to)
            continue;
        
        slotId -= from;
        
        aligned_advance_by(a, (sizeof(PS_PPItem) * slotId) + sizeof(uint16_t));
        aligned_write_uint8(a, (uint8_t)slot->item->charges);
        aligned_reset_cursor_to(a, resetTo);
    }
    
    aligned_advance_by(a, diff * sizeof(PS_PPItem));
}

void inv_write_pp_main_item_ids(Inventory* inv, Aligned* a)
{
    inv_write_pp_item_ids(inv, a, INV_SLOT_EquipMainAndCursorBegin, INV_SLOT_EquipMainAndCursorEnd);
}

void inv_write_pp_main_item_properties(Inventory* inv, Aligned* a)
{
    inv_write_pp_item_properties(inv, a, INV_SLOT_EquipMainAndCursorBegin, INV_SLOT_EquipMainAndCursorEnd);
}

void inv_write_pp_main_bag_item_ids(Inventory* inv, Aligned* a)
{
    inv_write_pp_item_ids(inv, a, INV_SLOT_BagsSlotsIncludingCursorBegin, INV_SLOT_BagsSlotsIncludingCursorEnd);
}

void inv_write_pp_main_bag_item_properties(Inventory* inv, Aligned* a)
{
    inv_write_pp_item_properties(inv, a, INV_SLOT_BagsSlotsIncludingCursorBegin, INV_SLOT_BagsSlotsIncludingCursorEnd);
}

void inv_write_pp_bank_item_ids(Inventory* inv, Aligned* a)
{
    inv_write_pp_item_ids(inv, a, INV_SLOT_BankBegin, INV_SLOT_BankEnd);
}

void inv_write_pp_bank_item_properties(Inventory* inv, Aligned* a)
{
    inv_write_pp_item_properties(inv, a, INV_SLOT_BankBegin, INV_SLOT_BankEnd);
}

void inv_write_pp_bank_bag_item_ids(Inventory* inv, Aligned* a)
{
    inv_write_pp_item_ids(inv, a, INV_SLOT_BankBagSlotsBegin, INV_SLOT_BankBagSlotsEnd);
}

void inv_write_pp_bank_bag_item_properties(Inventory* inv, Aligned* a)
{
    inv_write_pp_item_properties(inv, a, INV_SLOT_BankBagSlotsBegin, INV_SLOT_BankBagSlotsEnd);
}
