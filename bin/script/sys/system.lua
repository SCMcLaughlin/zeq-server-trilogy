
--------------------------------------------------------------------------------
-- sys/zone_thread/system.lua
--------------------------------------------------------------------------------
--
-- The functions in this package are responsible for creating and managing
-- lua-side references to C-side objects. Most of the interfaces to the lua
-- system which are callable from the C side are also defined here.
--
-- This script is automatically run once whenever a new ZoneThread is created.
--
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
-- Imports
--------------------------------------------------------------------------------
local ffi = require "ffi"
local ZoneThread = require "sys/ZoneThread"
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
-- Lazy imports
--------------------------------------------------------------------------------
local Zone
local Client
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
-- Caches
--------------------------------------------------------------------------------
local C = ffi.C
local setmetatable = setmetatable
local setfenv = setfenv
local xpcall = xpcall
local traceback = debug.traceback
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
-- Object lists
--------------------------------------------------------------------------------
local objects = {}
local mobsByZone = {}
local clientsByZone = {}
local npcsByZone = {}
local zonesByMob = {}
local timers = setmetatable({}, {__mode = "v"})
--------------------------------------------------------------------------------

local system = {}

function system.createZoneThread(ptr)
    local obj = {
        _ptr = ptr,
        __index = ZoneThread,
    }
    
    setmetatable(obj, obj)
    
    -- The ZoneThread object is available from any script, through require "ZoneThread" or require "ZT"
    package.loaded["ZoneThread"] = obj
    package.loaded["ZT"] = obj
end

local function removeObject(index)
    local obj = objects[index]
    
    if obj then
        obj._ptr = nil
        objects[index] = nil
        return obj
    end
end

local function getObject(index)
    return objects[index]
end

system.removeObject = removeObject
system.getObject = getObject

local function addObject(obj)
    local index = #objects + 1
    objects[index] = obj
    return index
end

function system.createZone(ptr)
    if not Zone then
        Zone = require "sys/Zone"
    end
    
    local zone = Zone(ptr)
    mobsByZone[zone] = {}
    clientsByZone[zone] = {}
    npcsByZone[zone] = {}
    
    return addObject(zone)
end

function system.removeZone(index)
    local zone = removeObject(index)
    if not zone then return end
    
    mobsByZone[zone] = nil
    clientsByZone[zone] = nil
    npcsByZone[zone] = nil
end

function system.addTimer(timer)
    local index = #timers + 1
    timers[index] = timer
    return index
end

function system.removeTimer(index)
    local timer = timers[index]
    
    if timer then
        timer._ptr = nil
        timers[index] = nil
        return timer
    end
end

function system.timerCallback(index)
    local timer = timers[index]
    
    if timer then
        timer._func(timer)
    end
end

return system
