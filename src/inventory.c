
#include "inventory.h"
#include "aligned.h"
#include "bit.h"
#include "client.h"
#include "client_packet_send.h"
#include "client_save.h"
#include "packet_create.h"
#include "player_profile_packet_struct.h"
#include "util_alloc.h"

#define INV_CLIENT_ITEM_ID_OFFSET 1000

void inv_init(Inventory* inv)
{
    inv->primaryIndex = -1;
    inv->secondaryIndex = -1;
    inv->rangeIndex = -1;
    inv->ammoIndex = -1;
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

static int inv_from_db_to_slot(Inventory* inv, Item* item, uint16_t slotId, uint32_t itemId)
{
    uint16_t index;
    InvSlot* dst;
    
    if (slotId == INV_SLOT_Cursor)
    {
        dst = &inv->cursor;
        goto set;
    }
    
    index = inv->slotCount;
    
    if (bit_is_pow2_or_zero(index))
    {
        uint32_t cap = (index == 0) ? 1 : index * 2;
        InvSlot* slots = realloc_array_type(inv->slots, cap, InvSlot);
        
        if (!slots) return ERR_OutOfMemory;
        
        inv->slots = slots;
    }
    
    dst = &inv->slots[index];
    inv->slotCount = index + 1;
    
    switch (slotId)
    {
    case INV_SLOT_Primary:
        inv->primaryIndex = (int16_t)index;
        break;
    
    case INV_SLOT_Secondary:
        inv->secondaryIndex = (int16_t)index;
        break;
    
    case INV_SLOT_Range:
        inv->rangeIndex = (int16_t)index;
        break;
    
    case INV_SLOT_Ammo:
        inv->ammoIndex = (int16_t)index;
        break;
    
    default:
        break;
    }
    
set:
    dst->slotId = slotId;
    dst->itemId = itemId;
    dst->item = item;
    return ERR_None;
}

int inv_from_db(Inventory* inv, ClientLoadData_Inventory* data, uint32_t count, ItemList* itemList)
{
    uint32_t i;
    int rc;
    
    for (i = 0; i < count; i++)
    {
        ClientLoadData_Inventory* slot = &data[i];
        ItemProto* proto = item_list_by_id(itemList, slot->itemId);
        Item* item;
        
        if (!proto) continue;
        
        item = alloc_type(Item);
        if (!item) goto oom;
        
        item->proto = proto;
        item->stackAmt = slot->stackAmt;
        item->charges = slot->charges;
        /*item->isBag = ;*/
        
        rc = inv_from_db_to_slot(inv, item, slot->slotId, slot->itemId);
        if (rc)
        {
            free(item);
            goto oom;
        }
    }
    
    return ERR_None;
oom:
    return ERR_OutOfMemory;
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

void inv_send_all(Inventory* inv, Client* client, RingBuf* udpQueue)
{
    InvSlot* slots = inv->slots;
    uint32_t n = inv->slotCount;
    uint32_t i;
    
    for (i = 0; i < n; i++)
    {
        InvSlot* slot = &slots[i];
        Item* item = slot->item;
        ItemProto* proto = item->proto;
        TlgPacket* packet = packet_create_inv_item(item, proto, slot->slotId, inv_item_id_for_client(inv, item_proto_item_id(proto)));
        
        if (packet)
            client_schedule_packet_with_udp_queue(client, udpQueue, packet);
    }
}

static ClientSaveInvSlot* inv_save_slots(InvSlot* slots, uint32_t n)
{
    ClientSaveInvSlot* saveSlots;
    uint32_t i;
    
    saveSlots = alloc_array_type(n, ClientSaveInvSlot);
    if (!saveSlots) return NULL;

    for (i = 0; i < n; i++)
    {
        InvSlot* slot = &slots[i];
        Item* item = slot->item;
        ClientSaveInvSlot* saveSlot = &saveSlots[i];

        saveSlot->itemId = slot->itemId; 
        saveSlot->slotId = slot->slotId;
        saveSlot->charges = item->charges;
        saveSlot->stackAmt = item->stackAmt;
    }

    return saveSlots;
}

int inv_save_all(Inventory* inv, ClientSave* save)
{
    uint32_t n = inv->slotCount;

    save->itemCount = n;

    if (n)
    {
        save->items = inv_save_slots(inv->slots, n);
        if (!save->items) goto oom;
    }

    n = inv->cursorCount;
    save->cursorQueueCount = n;

    if (n)
    {
        save->cursorQueue = inv_save_slots(inv->cursorQueue, n);
        if (!save->cursorQueue) goto oom;
    }

    return ERR_None;

oom:
    return ERR_OutOfMemory;
}

void inv_calc_stats(Inventory* inv, CoreStats* stats, uint32_t* weight, int* acFromItems)
{
    InvSlot* slots = inv->slots;
    uint32_t n = inv->slotCount;
    uint32_t i;
    
    for (i = 0; i < n; i++)
    {
        InvSlot* slot = &slots[i];
        uint16_t slotId = slot->slotId;
        ItemProto* proto;
        
        if (slotId < INV_SLOT_EquipMainAndCursorBegin || slotId > INV_SLOT_EquipMainAndCursorEnd)
            continue;
        
        proto = slot->item->proto;
        
        if (slotId <= INV_SLOT_EquipForStatsEnd)
            item_proto_calc_stats(proto, stats, weight, acFromItems);
        else
            *weight += item_proto_weight(proto);
    }
}

static void inv_write_pp_item_ids(Inventory* inv, Aligned* a, uint32_t from, uint32_t to)
{
    uint32_t diff = (to - from) + 1;
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
    uint32_t diff = (to - from) + 1;
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
        aligned_write_uint8(a, slot->item->charges);
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
