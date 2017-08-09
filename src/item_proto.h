
#ifndef ITEM_PROTO_H
#define ITEM_PROTO_H

#include "define.h"
#include "util_hash_tbl.h"

typedef struct ItemProto ItemProto;

typedef struct {
    StaticBuffer*   path;
    uint32_t        itemId;
} ItemPathToItemId;

typedef struct {
    uint32_t            nextItemId;
    uint32_t            pathToItemIdCount;
    HashTbl             tblByPath;
    HashTbl             tblPathToItemId;
    ItemPathToItemId*   pathToItemId;
} ItemList;

void item_list_init(ItemList* itemList);
void item_list_deinit(ItemList* itemList);

ZEQ_API ItemProto* item_proto_add(ItemList* itemList, const char* path, uint32_t len);

ZEQ_API bool item_proto_set_name(ItemProto* proto, const char* name, uint32_t len);
ZEQ_API bool item_proto_set_lore(ItemProto* proto, const char* lore, uint32_t len);
ZEQ_API bool item_proto_set_field(ItemProto* proto, int16_t statId, int16_t value);
ZEQ_API void item_proto_set_slots(ItemProto* proto, uint32_t slotsBitfield);

#endif/*ITEM_PROTO_H*/
