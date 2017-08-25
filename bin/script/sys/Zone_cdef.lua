
local ffi = require "ffi"

ffi.cdef[[
const char* zone_short_name(void* zone);
const char* zone_long_name(void* zone);
int zone_lua_index(void* zone);

void zone_broadcast_to_all_clients_except(void* zone, void* packet, void* except);
void zone_broadcast_to_nearby_clients_except(void* zone, void* packet, double x, double y, double z, double range, void* except);

void* zone_log_queue(void* zone);
int zone_log_id(void* zone);
]]

return ffi.C
