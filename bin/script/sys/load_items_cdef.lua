
local ffi = require "ffi"

ffi.cdef[[
typedef struct ItemProto ItemProto;

ItemProto* item_proto_add(void* itemList, const char* path, uint32_t len);

bool item_proto_set_name(ItemProto* proto, const char* name, uint32_t len);
bool item_proto_set_lore(ItemProto* proto, const char* lore, uint32_t len);
bool item_proto_set_field(ItemProto* proto, int16_t statId, int16_t value);
]]

return ffi.C
