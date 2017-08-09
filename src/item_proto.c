
#include "item_proto.h"
#include "bit.h"
#include "buffer.h"
#include "util_alloc.h"
#include "util_hash_tbl.h"

typedef struct {
    int16_t statId;
    int16_t value;
} ItemField;

struct ItemProto {
    uint32_t        fieldCount;
    uint32_t        itemId;
    uint32_t        slots;
    StaticBuffer*   name;
    StaticBuffer*   lore;
    StaticBuffer*   path;
    ItemField*      fields;
};

void item_list_init(ItemList* itemList)
{
    tbl_init(&itemList->tblByPath, ItemProto*);
    tbl_init(&itemList->tblPathToItemId, ItemPathToItemId);
}

static void item_proto_destroy(void* ptr)
{
    ItemProto* proto = *((ItemProto**)ptr);

    proto->name = sbuf_drop(proto->name);
    proto->lore = sbuf_drop(proto->lore);
    proto->path = sbuf_drop(proto->path);
    free_if_exists(proto->fields);
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
    tbl_deinit(&itemList->tblPathToItemId, item_path2id_free);
}

ItemProto* item_proto_add(ItemList* itemList, const char* path, uint32_t len)
{
    ItemProto* proto;
    StaticBuffer* sbufPath;
    uint32_t* itemId;
    int rc;

    sbufPath = sbuf_create(path, len);
    if (!sbufPath) return NULL;
    sbuf_grab(sbufPath);

    proto = alloc_type(ItemProto);
    if (!proto) goto fail_alloc;

    proto->fieldCount = 0;
    proto->name = NULL;
    proto->lore = NULL;
    proto->path = sbufPath;
    proto->fields = NULL;

    itemId = tbl_get_str(&itemList->tblPathToItemId, sbuf_str(sbufPath), sbuf_length(sbufPath), uint32_t);

    if (itemId)
    {
        proto->itemId = *itemId;
    }
    else
    {
        uint32_t index = itemList->pathToItemIdCount;
        uint32_t id;

        if (bit_is_pow2_or_zero(index))
        {
            uint32_t cap = (index == 0) ? 1 : index * 2;
            ItemPathToItemId* pathToItemId = realloc_array_type(itemList->pathToItemId, cap, ItemPathToItemId);

            if (!pathToItemId) goto fail;

            itemList->pathToItemId = pathToItemId;
        }

        id = itemList->nextItemId++;
        sbuf_grab(sbufPath);

        itemList->pathToItemId[index].path = sbufPath;
        itemList->pathToItemId[index].itemId = id;
        itemList->pathToItemIdCount = index + 1;
    }
    
    rc = tbl_set_sbuf(&itemList->tblByPath, sbufPath, (void*)&proto);
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
    proto->name = sbuf_drop(proto->name);
    proto->name = sbuf_create(name, len);
    return proto->name != NULL;
}

bool item_proto_set_lore(ItemProto* proto, const char* lore, uint32_t len)
{
    proto->lore = sbuf_drop(proto->lore);
    proto->lore = sbuf_create(lore, len);
    return proto->lore != NULL;
}

bool item_proto_set_field(ItemProto* proto, int16_t statId, int16_t value)
{
    uint32_t index = proto->fieldCount;

    if (bit_is_pow2_or_zero(index))
    {
        uint32_t cap = (index == 0) ? 1 : index * 2;
        ItemField* fields = realloc_array_type(proto->fields, cap, ItemField);

        if (!fields) return false;

        proto->fields = fields;
    }

    proto->fields[index].statId = statId;
    proto->fields[index].value = value;
    proto->fieldCount = index + 1;
    return true;
}

void item_proto_set_slots(ItemProto* proto, uint32_t slotsBitfield)
{
    proto->slots = slotsBitfield;
}
