
#include "item_proto.h"
#include "bit.h"
#include "buffer.h"
#include "util_alloc.h"
#include "util_hash_tbl.h"

#ifdef PLATFORM_WINDOWS
/* fields[0]: "nonstandard extension used: zero-sized array in struct/union" */
# pragma warning(disable: 4200)
#endif

typedef struct {
    int16_t statId;
    int16_t value;
} ItemField;

struct ItemProto {
    uint16_t        fieldCount;
    uint32_t        itemId;
    uint32_t        slots;
    StaticBuffer*   name;
    StaticBuffer*   lore;
    StaticBuffer*   path;
    ItemField       fields[0];
};

void item_list_init(ItemList* itemList)
{
    tbl_init(&itemList->tblByPath, ItemProto*);
    tbl_init(&itemList->tblByItemId, ItemProto*);
    tbl_init(&itemList->tblPathToItemId, ItemPathToItemId);
}

static void item_proto_destroy(void* ptr)
{
    ItemProto* proto = *((ItemProto**)ptr);

    proto->name = sbuf_drop(proto->name);
    proto->lore = sbuf_drop(proto->lore);
    proto->path = sbuf_drop(proto->path);
    free(proto);
}

static void item_path2id_free(void* ptr)
{
    ItemPathToItemId* to = (ItemPathToItemId*)ptr;

    to->path = sbuf_drop(to->path);
}

void item_list_deinit(ItemList* itemList)
{
    tbl_deinit(&itemList->tblByPath, item_proto_destroy);
    tbl_deinit(&itemList->tblByItemId, NULL);
    tbl_deinit(&itemList->tblPathToItemId, item_path2id_free);
}

ItemProto* item_proto_add(ItemList* itemList, const char* path, uint32_t len, uint16_t fieldCount)
{
    ItemProto* proto;
    StaticBuffer* sbufPath;
    uint32_t* itemId;
    uint32_t bytes;
    int rc;

    sbufPath = sbuf_create(path, len);
    if (!sbufPath) return NULL;
    sbuf_grab(sbufPath);

    bytes = sizeof(ItemProto) + (sizeof(ItemField) * fieldCount);
    proto = alloc_bytes_type(bytes, ItemProto);
    if (!proto) goto fail_alloc;

    memset(proto, 0, bytes);
    proto->fieldCount = fieldCount;
    proto->path = sbufPath;

    itemId = tbl_get_str(&itemList->tblPathToItemId, sbuf_str(sbufPath), sbuf_length(sbufPath), uint32_t);

    if (itemId)
    {
        proto->itemId = *itemId;
    }
    else
    {
        uint32_t index = itemList->newPathsToItemIdsCount;
        uint32_t id;

        if (bit_is_pow2_or_zero(index))
        {
            uint32_t cap = (index == 0) ? 1 : index * 2;
            ItemPathToItemId* pathToItemId = realloc_array_type(itemList->newPathsToItemIds, cap, ItemPathToItemId);

            if (!pathToItemId) goto fail;

            itemList->newPathsToItemIds = pathToItemId;
        }

        id = itemList->nextItemId++;
        sbuf_grab(sbufPath);

        itemList->newPathsToItemIds[index].path = sbufPath;
        itemList->newPathsToItemIds[index].itemId = id;
        itemList->newPathsToItemIdsCount = index + 1;
        
        proto->itemId = id;
    }
    
    rc = tbl_set_sbuf(&itemList->tblByPath, sbufPath, (void*)&proto);
    if (rc) goto fail;
    
    rc = tbl_set_int(&itemList->tblByItemId, (int64_t)proto->itemId, (void*)&proto);
    if (rc) goto fail;

    return proto;

fail:
    free(proto);
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

bool item_proto_set_field(ItemProto* proto, uint16_t index, int16_t statId, int16_t value)
{
    if (index >= proto->fieldCount)
        return false;
    
    proto->fields[index].statId = statId;
    proto->fields[index].value = value;
    return true;
}

void item_proto_set_slots(ItemProto* proto, uint32_t slotsBitfield)
{
    proto->slots = slotsBitfield;
}
