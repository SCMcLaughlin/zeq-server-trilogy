
local ffi = require "ffi"

ffi.cdef[[
int log_write(void* logQueue, int logId, const char* msg, int len);
]]

return ffi.C
