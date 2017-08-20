
--------------------------------------------------------------------------------
-- Imports
--------------------------------------------------------------------------------
local C = require "sys/ZoneThread_cdef"
local Class = require "sys/class"
local Timer = require "sys/Timer"

require "sys/log_cdef"
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
-- Caches
--------------------------------------------------------------------------------
local format = string.format
local rawset = rawset
local rawget = rawget
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

function ZoneThread:timer(periodMs, func)
    local timers = rawget(self, "_timers")
    
    if not timers then
        timers = {}
        rawset(self, "_timers", timers)
    end
    
    local timer = Timer(self, self, periodMs, func)
    timers[timer] = true
end

function ZoneThread:_timerDestroyed(timer)
    local timers = rawget(self, "_timers")
    
    if timers then
        timers[timer] = nil
    end
end

return ZoneThread
