
#ifndef ITEM_PROTO_H
#define ITEM_PROTO_H

#include "define.h"
#include "buffer.h"
#include "util_hash_tbl.h"

typedef struct ItemProto ItemProto;

typedef struct {
    StaticBuffer*   path;
    uint32_t        itemId;
} ItemPathToItemId;

typedef struct {
    uint64_t        modTime;
    uint32_t        itemId;
    StaticBuffer*   path;
    ItemProto*      proto;
} ItemProtoDb;

typedef struct {
    uint32_t            nextItemId;
    uint32_t            newPathsToItemIdsCount;
    HashTbl             tblByPath;
    HashTbl             tblByItemId;
    HashTbl             tblPathToItemId;
    ItemPathToItemId*   newPathsToItemIds;
} ItemList;

void item_list_init(ItemList* itemList);
void item_list_deinit(ItemList* itemList);

ZEQ_API ItemProto* item_proto_add(ItemList* itemList, const char* path, uint32_t len, uint16_t fieldCount);

ZEQ_API bool item_proto_set_name(ItemProto* proto, const char* name, uint32_t len);
ZEQ_API bool item_proto_set_lore(ItemProto* proto, const char* lore, uint32_t len);
ZEQ_API bool item_proto_set_field(ItemProto* proto, uint16_t index, uint8_t statId, int value);

ItemProto* item_proto_from_db(const byte* blob, int len, StaticBuffer* path, StaticBuffer* name, StaticBuffer* loreText);
ItemProto* item_proto_from_db_destroy(ItemProto* proto);

#endif/*ITEM_PROTO_H*/
