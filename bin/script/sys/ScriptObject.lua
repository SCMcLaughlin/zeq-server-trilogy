
--------------------------------------------------------------------------------
-- Imports
--------------------------------------------------------------------------------
local Class = require "sys/class"
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
-- Caches
--------------------------------------------------------------------------------
local getmetatable = getmetatable
local error = error
local rawget = rawget
local rawset = rawset
local pairs = pairs
--------------------------------------------------------------------------------

local ScriptObject = Class("ScriptObject")

function ScriptObject:ptr()
    local ptr = self._ptr
    if ptr == nil then
        error "Attempt to use an object that has been destroyed on the C side"
    end
    return ptr
end

function ScriptObject:isValid()
    return self._ptr ~= nil
end

--------------------------------------------------------------------------------
-- exists() is an alias for isValid()
--------------------------------------------------------------------------------
ScriptObject.exists = ScriptObject.isValid

function ScriptObject:getClassName()
    return "ScriptObject"
end

function ScriptObject:getPersonalEnvironment()
    return getmetatable(self)
end

function ScriptObject:timer(logger, periodMs, func)
    local timers = rawget(self, "_timers")
    
    if not timers then
        timers = {}
        rawset(self, "_timers", timers)
    end
    
    local timer = Timer(logger, self, periodMs, func)
    timers[timer] = true
end

function ScriptObject:_timerDestroyed(timer)
    local timers = rawget(self, "_timers")
    
    if timers then
        timers[timer] = nil
    end
end

function ScriptObject:_timerStopAll()
    local timers = rawget(self, "_timers")
    
    if timers then
        for timer in pairs(timers) do
            timer:stop()
        end
    end
end

return ScriptObject
