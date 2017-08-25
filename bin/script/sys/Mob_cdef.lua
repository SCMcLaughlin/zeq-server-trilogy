
local ffi = require "ffi"

ffi.cdef[[
const char* mob_name_str(void* mob);
int mob_lua_index(void* mob);
double mob_cur_size(void* mob);
void* mob_target(void* mob);
void mob_update_level(void* mob, uint8_t level);
void mob_update_size(void* mob, double size);
]]

return ffi.C
