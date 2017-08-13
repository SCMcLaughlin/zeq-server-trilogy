
#include "item_proto.h"
#include "bit.h"
#include "buffer.h"
#include "item_client_type_id.h"
#include "item_stat_id.h"
#include "item_type_id.h"
#include "util_alloc.h"
#include "util_cap.h"
#include "util_hash_tbl.h"

#ifdef PLATFORM_WINDOWS
/* data[0]: "nonstandard extension used: zero-sized array in struct/union" */
# pragma warning(disable: 4200)
#endif

struct ItemProto {
    uint16_t        fieldCount;
    uint32_t        itemId;
    StaticBuffer*   name;
    StaticBuffer*   lore;
    StaticBuffer*   path;
    byte            data[0]; /* A list of 4-byte values followed by a list of 1-byte stat ids */
};

void item_list_init(ItemList* itemList)
{
    tbl_init(&itemList->tblByPath, ItemProto*);
    tbl_init(&itemList->tblByItemId, ItemProto*);
}

ItemProto* item_proto_destroy(ItemProto* proto)
{
    if (proto)
    {
        proto->name = sbuf_drop(proto->name);
        proto->lore = sbuf_drop(proto->lore);
        proto->path = sbuf_drop(proto->path);
        free(proto);
    }

    return NULL;
}

static void item_proto_tbl_destroy(void* ptr)
{
    ItemProto* proto = *((ItemProto**)ptr);

    item_proto_destroy(proto);
}

void item_list_deinit(ItemList* itemList)
{
    tbl_deinit(&itemList->tblByPath, item_proto_tbl_destroy);
    tbl_deinit(&itemList->tblByItemId, NULL);
}

ItemProto* item_list_by_id(ItemList* itemList, uint32_t itemId)
{
    ItemProto** proto = tbl_get_int(&itemList->tblByItemId, (int64_t)itemId, ItemProto*);
    return (proto) ? *proto : NULL;
}

int item_proto_add_from_db(ItemList* itemList, ItemProto* proto, uint32_t itemId)
{
    StaticBuffer* path = proto->path;
    int rc;

    rc = tbl_set_sbuf(&itemList->tblByPath, path, (void*)&proto);
    if (rc) goto fail;

    rc = tbl_set_int(&itemList->tblByItemId, (int64_t)itemId, (void*)&proto);
    if (rc) goto fail;

fail:
    return rc;
}

ItemProto* item_proto_add(ItemList* itemList, ItemProtoDbChanges* changes, uint64_t modTime, uint32_t itemId, const char* path, uint32_t len, uint16_t fieldCount)
{
    ItemProto* proto;
    StaticBuffer* sbufPath;
    uint32_t bytes;
    uint32_t index;
    int rc;

    sbufPath = sbuf_create(path, len);
    if (!sbufPath) return NULL;
    sbuf_grab(sbufPath);

    bytes = sizeof(ItemProto) + sizeof(int) * fieldCount + sizeof(uint8_t) * fieldCount;
    proto = alloc_bytes_type(bytes, ItemProto);
    if (!proto) goto fail_alloc;

    if (itemId == 0)
        itemId = itemList->nextItemId++;
    
    proto->fieldCount = fieldCount;
    proto->itemId = itemId;
    proto->name = NULL;
    proto->lore = NULL;
    proto->path = sbufPath;

    rc = item_proto_add_from_db(itemList, proto, itemId);
    if (rc) goto fail;

    index = changes->count;

    if (bit_is_pow2_or_zero(index))
    {
        uint32_t cap = (index == 0) ? 1 : index * 2;
        ItemProtoDb* ipd = realloc_array_type(changes->proto, cap, ItemProtoDb);

        if (!ipd) goto fail;

        changes->proto = ipd;
    }

    changes->proto[index].modTime = modTime;
    changes->proto[index].itemId = itemId;
    changes->proto[index].path = sbufPath;
    changes->proto[index].proto = proto;
    changes->count = index + 1;

    return proto;

fail:
    item_proto_destroy(proto);
fail_alloc:
    sbuf_drop(sbufPath);
    return NULL;
}

bool item_proto_set_name(ItemProto* proto, const char* name, uint32_t len)
{
    proto->name = sbuf_create(name, len);
    sbuf_grab(proto->name);
    return proto->name != NULL;
}

bool item_proto_set_lore(ItemProto* proto, const char* lore, uint32_t len)
{
    proto->lore = sbuf_create(lore, len);
    sbuf_grab(proto->lore);
    return proto->lore != NULL;
}

#define item_proto_field_value(proto, n) ((int*)(&((proto)->data[(n) * sizeof(int)])))
#define item_proto_field_id(proto, n) (&((proto)->data[(proto)->fieldCount * sizeof(int) + (n)]))

bool item_proto_set_field(ItemProto* proto, uint16_t index, uint8_t statId, int value)
{
    if (index >= proto->fieldCount)
        return false;
    
    *item_proto_field_value(proto, index) = value;
    *item_proto_field_id(proto, index) = statId;
    return true;
}

ItemProto* item_proto_from_db(const byte* blob, int len, StaticBuffer* path, StaticBuffer* name, StaticBuffer* loreText)
{
    const ItemProto* cproto = (const ItemProto*)blob;
    uint32_t bytes;
    ItemProto* proto;
    
    if (!blob || len < (int)sizeof(ItemProto))
        return NULL;
    
    bytes = sizeof(ItemProto) + sizeof(int) * cproto->fieldCount + sizeof(uint8_t) * cproto->fieldCount;
    
    if (len < (int)bytes)
        return NULL;
    
    proto = alloc_bytes_type(bytes, ItemProto);
    if (!proto) return NULL;
    
    memcpy(proto, cproto, sizeof(ItemProto));
    proto->path = path;
    proto->name = name;
    proto->lore = loreText;
    
    sbuf_grab(path);
    sbuf_grab(name);
    sbuf_grab(loreText);
    
    return proto;
}

uint32_t item_proto_bytes(ItemProto* proto)
{
    uint16_t fieldCount = proto->fieldCount;
    return sizeof(ItemProto) + sizeof(int) * fieldCount + sizeof(uint8_t) * fieldCount;
}

StaticBuffer* item_proto_path(ItemProto* proto)
{
    return proto->path;
}

StaticBuffer* item_proto_name(ItemProto* proto)
{
    return proto->name;
}

StaticBuffer* item_proto_lore_text(ItemProto* proto)
{
    return proto->lore;
}

uint32_t item_proto_item_id(ItemProto* proto)
{
    return proto->itemId;
}

static void item_proto_to_packet_handle_item_type(PC_Item* item, int value)
{
    switch (value)
    {
    case ITEM_TYPE_Generic:
        item->itemType = ITEM_CLIENT_TYPE_Armor;
        break;
    
    case ITEM_TYPE_Stackable:
        item->isStackable = true,
        item->itemType = ITEM_CLIENT_TYPE_Stackable;
        break;
    
    case ITEM_TYPE_Bag:
    case ITEM_TYPE_TradeskillBag:
        item->isBag = true;
        item->itemType = ITEM_CLIENT_TYPE_Armor;
        break;
    
    case ITEM_TYPE_Book:
        item->isBook = true;
        item->itemType = ITEM_CLIENT_TYPE_Book;
        break;
    
    case ITEM_TYPE_Note:
        item->isBook = true;
        item->itemType = ITEM_CLIENT_TYPE_Note;
        break;
    
    case ITEM_TYPE_Scroll:
        item->isScroll = true;
        item->itemType = ITEM_CLIENT_TYPE_SpellScroll;
        break;
    
    case ITEM_TYPE_Food:
        item->isStackable = true;
        item->itemType = ITEM_CLIENT_TYPE_Food;
        break;
    
    case ITEM_TYPE_Drink:
        item->isStackable = true;
        item->itemType = ITEM_CLIENT_TYPE_Drink;
        break;
    
    case ITEM_TYPE_1HSlash:
        item->itemType = ITEM_CLIENT_TYPE_1HSlash;
        break;
    
    case ITEM_TYPE_2HSlash:
        item->itemType = ITEM_CLIENT_TYPE_2HSlash;
        break;
    
    case ITEM_TYPE_1HBlunt:
        item->itemType = ITEM_CLIENT_TYPE_1HBlunt;
        break;
    
    case ITEM_TYPE_2HBlunt:
        item->itemType = ITEM_CLIENT_TYPE_2HBlunt;
        break;
    
    case ITEM_TYPE_1HPiercing:
        item->itemType = ITEM_CLIENT_TYPE_1HPiercing;
        break;
    
    case ITEM_TYPE_2HPiercing:
        item->itemType = ITEM_CLIENT_TYPE_2HPiercing;
        break;
    
    case ITEM_TYPE_Archery:
        item->itemType = ITEM_CLIENT_TYPE_Archery;
        break;
    
    case ITEM_TYPE_Arrow:
        item->isStackable = true;
        item->itemType = ITEM_CLIENT_TYPE_Arrow;
        break;
    
    case ITEM_TYPE_Throwing:
        item->itemType = ITEM_CLIENT_TYPE_Throwing;
        break;
    
    case ITEM_TYPE_ThrowingStackable:
        item->isStackable = true;
        item->itemType = ITEM_CLIENT_TYPE_ThrowingStackable;
        break;
    
    case ITEM_TYPE_Shield:
        item->itemType = ITEM_CLIENT_TYPE_Bash;
        break;
    
    case ITEM_TYPE_Lockpick:
        item->itemType = ITEM_CLIENT_TYPE_Lockpick;
        break;
    
    case ITEM_TYPE_Bandage:
        item->itemType = ITEM_CLIENT_TYPE_Bandage;
        break;
    
    case ITEM_TYPE_WindInstrument:
        item->itemType = ITEM_CLIENT_TYPE_WindInstrument;
        break;
    
    case ITEM_TYPE_StringInstrument:
        item->itemType = ITEM_CLIENT_TYPE_StringInstrument;
        break;
    
    case ITEM_TYPE_BrassInstrument:
        item->itemType = ITEM_CLIENT_TYPE_BrassInstrument;
        break;
    
    case ITEM_TYPE_PercussionInstrument:
        item->itemType = ITEM_CLIENT_TYPE_PercussionInstrument;
        break;
    
    case ITEM_TYPE_Singing:
        item->itemType = ITEM_CLIENT_TYPE_Singing;
        break;
    
    case ITEM_TYPE_AllInstruments:
        item->itemType = ITEM_CLIENT_TYPE_AllInstruments;
        break;
    
    case ITEM_TYPE_Key:
        item->itemType = ITEM_CLIENT_TYPE_Key;
        break;
    
    case ITEM_TYPE_FishingRod:
        item->itemType = ITEM_CLIENT_TYPE_FishingRod;
        break;
    
    case ITEM_TYPE_FishingBait:
        item->isStackable = true;
        item->itemType = ITEM_CLIENT_TYPE_FishingBait;
        break;
    
    case ITEM_TYPE_Alcohol:
        item->itemType = ITEM_CLIENT_TYPE_Alcohol;
        break;
    
    case ITEM_TYPE_Compass:
        item->itemType = ITEM_CLIENT_TYPE_Compass;
        break;
    
    case ITEM_TYPE_Poison:
        item->itemType = ITEM_CLIENT_TYPE_Poison;
        break;
    
    case ITEM_TYPE_HandToHand:
        item->itemType = ITEM_CLIENT_TYPE_HandToHand;
        break;
    
    default:
        break;
    }
}

void item_proto_to_packet(ItemProto* proto, PC_Item* item)
{
    uint32_t n = proto->fieldCount;
    uint32_t i;
    
    item->name = proto->name;
    item->lore = proto->lore;
    
    for (i = 0; i < n; i++)
    {
        uint8_t fieldId = *item_proto_field_id(proto, i);
        int value = *item_proto_field_value(proto, i);
        
        switch (fieldId)
        {
        case ITEM_STAT_Lore:
            item->isUnique = true;
            break;
        
        case ITEM_STAT_NoDrop:
            item->isDroppable = false;
            break;
        
        case ITEM_STAT_NoRent:
            item->isPermanent = false;
            break;
        
        case ITEM_STAT_Magic:
            item->isMagic = true;
            break;
        
        case ITEM_STAT_AC:
            item->AC = cap_int8(value);
            break;
        
        case ITEM_STAT_DMG:
            item->damage = cap_uint8(value);
            break;
        
        case ITEM_STAT_Delay:
            item->delay = cap_uint8(value);
            break;
        
        case ITEM_STAT_Range:
            item->range = cap_uint8(value);
            break;
        
        case ITEM_STAT_STR:
            item->STR = cap_int8(value);
            break;
        
        case ITEM_STAT_STA:
            item->STA = cap_int8(value);
            break;
        
        case ITEM_STAT_CHA:
            item->CHA = cap_int8(value);
            break;
        
        case ITEM_STAT_DEX:
            item->DEX = cap_int8(value);
            break;
        
        case ITEM_STAT_AGI:
            item->AGI = cap_int8(value);
            break;
        
        case ITEM_STAT_INT:
            item->INT = cap_int8(value);
            break;
        
        case ITEM_STAT_WIS:
            item->WIS = cap_int8(value);
            break;
        
        case ITEM_STAT_SvMagic:
            item->MR = cap_int8(value);
            break;
        
        case ITEM_STAT_SvFire:
            item->FR = cap_int8(value);
            break;
        
        case ITEM_STAT_SvCold:
            item->CR = cap_int8(value);
            break;
        
        case ITEM_STAT_SvPoison:
            item->PR = cap_int8(value);
            break;
        
        case ITEM_STAT_SvDisease:
            item->DR = cap_int8(value);
            break;
        
        case ITEM_STAT_Hp:
            item->hp = cap_int8(value);
            break;
        
        case ITEM_STAT_Mana:
            item->mana = cap_int8(value);
            break;
        
        case ITEM_STAT_Weight:
            item->weight = cap_uint8(value);
            break;
        
        case ITEM_STAT_Size:
            item->size = (uint8_t)cap_min_max(value, 0, 4);
            break;
        
        case ITEM_STAT_Slot:
            item->slots = (uint32_t)value;
            break;
        
        case ITEM_STAT_Race:
            item->races = (uint32_t)value;
            break;
        
        case ITEM_STAT_Class:
            item->classes = (uint32_t)value;
            break;
        
        case ITEM_STAT_BagSlots:
            if (value%2 != 0) value++; /* Must be multiple of 2 */
            item->bag.capacity = (uint8_t)cap_min_max(value, 0, 10);
            break;
        
        case ITEM_STAT_BagWeightReduction:
            item->bag.weightReductionPercent = (uint8_t)cap_min_max(value, 0, 100);
            break;
        
        case ITEM_STAT_BagMaxSize:
            item->bag.containableSize = (uint8_t)cap_min_max(value, 0, 4);
            break;
        
        case ITEM_STAT_SkillModSkillId:
        case ITEM_STAT_SkillModValue:
            break;
        
        case ITEM_STAT_Texture:
            item->materialId = (uint8_t)value;
            break;
        
        case ITEM_STAT_Tint:
            item->tint = (uint32_t)value;
            break;
        
        case ITEM_STAT_SpellId:
        case ITEM_STAT_SpellEffectType:
        case ITEM_STAT_SpellMinLevel:
        case ITEM_STAT_HasteMinLevel:
            break;
        
        case ITEM_STAT_Charges:
            item->isCharged = true;
            break;
        
        case ITEM_STAT_Haste:
            item->hastePercent = cap_uint8(value);
            break;
        
        case ITEM_STAT_Icon:
            item->icon = (uint16_t)cap_min_max(value, 500, 1264);
            break;
        
        case ITEM_STAT_VendorBuyPrice:
            item->cost = (uint32_t)value;
            break;
        
        case ITEM_STAT_Model:
            item->model = cap_uint8(value);
            break;
        
        case ITEM_STAT_ItemType:
            item_proto_to_packet_handle_item_type(item, value);
            break;
        
        case ITEM_STAT_Light:
            item->light = cap_uint8(value);
            break;
        
        default:
            break;
        }
    }
}
