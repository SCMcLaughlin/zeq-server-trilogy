
local ffi = require "ffi"

ffi.cdef[[
void* client_mob(void* client);
]]

return ffi.C
