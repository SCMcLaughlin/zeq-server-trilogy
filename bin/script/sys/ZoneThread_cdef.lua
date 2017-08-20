
local ffi = require "ffi"

ffi.cdef[[
typedef struct ZoneThread ZoneThread;

void* zt_get_log_queue(void* zt);
int zt_get_log_id(void* zt);

void* zt_timer_pool(void* zt);
void* zt_lua(void* zt);
]]

return ffi.C
