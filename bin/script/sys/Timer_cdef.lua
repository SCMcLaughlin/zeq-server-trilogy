
local ffi = require "ffi"

ffi.cdef[[
void* timer_new_lua(void* L, void* pool, uint32_t periodMs);
void* timer_destroy(void* pool, void* timer);
int timer_stop(void* pool, void* timer);
int timer_restart(void* pool, void* timer);
void timer_set_lua_index(void* timer, int luaIndex);
int timer_lua_index(void* timer);
]]

return ffi.C
