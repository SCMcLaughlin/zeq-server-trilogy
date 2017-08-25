
local ffi = require "ffi"

ffi.cdef[[
void* client_mob(void* client);
void client_update_level(void* client, uint8_t level);
void client_update_exp(void* client, uint32_t exp);
]]

return ffi.C
