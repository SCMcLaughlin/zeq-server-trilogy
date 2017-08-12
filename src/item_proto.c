
#include "item_proto.h"
#include "bit.h"
#include "buffer.h"
#include "util_alloc.h"
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
