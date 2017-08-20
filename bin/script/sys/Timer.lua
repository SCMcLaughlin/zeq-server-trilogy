
--------------------------------------------------------------------------------
-- Imports
--------------------------------------------------------------------------------
local C = require "sys/Timer_cdef"
local Class = require "sys/class"
local System = require "sys/system"
local ZT = require "ZoneThread"
local ffi = require "ffi"
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
-- Caches
--------------------------------------------------------------------------------
local TimerPool = C.zt_timer_pool(ZT:ptr())
local LuaState = C.zt_lua(ZT:ptr())
local xpcall = xpcall
local traceback = debug.traceback
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
-- In order to avoid an ownership ambiguity that would either a) prevent Timer
-- objects from ever being garbage collected, or b) have no way to determine
-- which Timers should be kept alive and which should be considered dead,
-- Timers should never be created standalone. Instead, they should always be
-- bound to a specific object, which they share their lifetime with.
--
-- In short, never use Timer() directly. Use ZoneThread:timer(), Zone:timer(),
-- Client:timer(), NPC:timer(), etc.
--------------------------------------------------------------------------------

local Timer = Class("Timer")

local function gcTimer(ptr)
    local index = C.timer_lua_index(ptr)
    
    local timer = System.removeTimer(index)
    C.timer_destroy(TimerPool, ptr)
    
    local owner = timer._owner
    
    owner:_timerDestroyed(timer)
end

function Timer.new(logger, owner, periodMs, func)
    logger = logger or ZT
    
    local ptr = C.timer_new_lua(LuaState, TimerPool, periodMs)
    if ptr == nil then error("Failed to allocate Timer object") end
    
    ffi.gc(ptr, gcTimer)
    
    local obj
    
    local function wrapper()
        local s, err = xpcall(func, obj)
        if not s then
            logger:log(err)
        end
    end
    
    obj = {
        _ptr = ptr,
        _func = wrapper,
        _owner = owner,
    }
    
    local index = System.addTimer(obj)
    C.timer_set_lua_index(ptr, index)
    
    return Timer:instance(obj)
end

function Timer:ptr()
    local ptr = self._ptr
    if ptr == nil then
        error "Attempt to use a Timer that has already been destroyed"
    end
    return ptr
end

function Timer:isValid()
    return self._ptr ~= nil
end

Timer.exists = Timer.isValid

function Timer:getClassName()
    return "Timer"
end

function Timer:stop()
    C.timer_stop(TimerPool, self:ptr())
end

function Timer:restart()
    C.timer_restart(TimerPool, self:ptr())
end

Timer.start = Timer.restart

return Timer
