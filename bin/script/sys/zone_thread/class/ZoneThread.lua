
--------------------------------------------------------------------------------
-- Imports
--------------------------------------------------------------------------------
local C = require "sys/zone_thread/cdef/ZoneThread_cdef"
local Class = require "sys/class"

require "sys/log_cdef"
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
-- Caches
--------------------------------------------------------------------------------
local format = string.format
local logQueue
local logId
--------------------------------------------------------------------------------

local ZoneThread = Class("ZoneThread")

function ZoneThread:ptr()
    return self._ptr
end

function ZoneThread:log(fmt, ...)
    local str = format(fmt, ...)
    
    if not logQueue then
        local zt = self:ptr()
        logQueue = C.zt_get_log_queue(zt)
        logId = C.zt_get_log_id(zt)
    end
    
    if str and #str > 0 then
        C.log_write(logQueue, logId, str, #str)
    end
end

return ZoneThread
