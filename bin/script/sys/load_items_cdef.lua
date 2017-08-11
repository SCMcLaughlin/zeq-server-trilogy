
local ffi = require "ffi"

ffi.cdef[[
typedef struct ItemProto ItemProto;

ItemProto* item_proto_add(void* itemList, void* changes, uint64_t modTime, uint32_t itemId, const char* path, uint32_t len, uint16_t fieldCount);

bool item_proto_set_name(ItemProto* proto, const char* name, uint32_t len);
bool item_proto_set_lore(ItemProto* proto, const char* lore, uint32_t len);
bool item_proto_set_field(ItemProto* proto, uint16_t index, uint8_t statId, int value);
]]

return ffi.C
