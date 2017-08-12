
#ifndef ITEM_PROTO_H
#define ITEM_PROTO_H

#include "define.h"
#include "buffer.h"
#include "packet_convert.h"
#include "util_hash_tbl.h"

typedef struct ItemProto ItemProto;

typedef struct {
    uint64_t        modTime;
    uint32_t        itemId;
    StaticBuffer*   path;
    ItemProto*      proto;
} ItemProtoDb;

typedef struct {
    uint32_t        count;
    ItemProtoDb*    proto;
} ItemProtoDbChanges;

typedef struct {
    uint32_t    nextItemId;
    HashTbl     tblByPath;
    HashTbl     tblByItemId;
} ItemList;

void item_list_init(ItemList* itemList);
void item_list_deinit(ItemList* itemList);

ZEQ_API ItemProto* item_proto_add(ItemList* itemList, ItemProtoDbChanges* changes, uint64_t modTime, uint32_t itemId, const char* path, uint32_t len, uint16_t fieldCount);

ZEQ_API bool item_proto_set_name(ItemProto* proto, const char* name, uint32_t len);
ZEQ_API bool item_proto_set_lore(ItemProto* proto, const char* lore, uint32_t len);
ZEQ_API bool item_proto_set_field(ItemProto* proto, uint16_t index, uint8_t statId, int value);
ItemProto* item_proto_destroy(ItemProto* proto);

ItemProto* item_proto_from_db(const byte* blob, int len, StaticBuffer* path, StaticBuffer* name, StaticBuffer* loreText);
int item_proto_add_from_db(ItemList* itemList, ItemProto* proto, uint32_t itemId);

uint32_t item_proto_bytes(ItemProto* proto);
StaticBuffer* item_proto_path(ItemProto* proto);
StaticBuffer* item_proto_name(ItemProto* proto);
StaticBuffer* item_proto_lore_text(ItemProto* proto);
uint32_t item_proto_item_id(ItemProto* proto);

void item_proto_to_packet(ItemProto* proto, PC_Item* item);

#endif/*ITEM_PROTO_H*/
