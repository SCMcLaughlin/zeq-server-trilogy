
local ffi = require "ffi"

ffi.cdef[[
const char* zone_short_name(void* zone);
const char* zone_long_name(void* zone);

void* zone_log_queue(void* zone);
int zone_log_id(void* zone);
]]

return ffi.C
