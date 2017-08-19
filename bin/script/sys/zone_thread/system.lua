
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
local ZoneThread = require "ZoneThread"
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
-- Lazy imports
--------------------------------------------------------------------------------
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
local clients = {}
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

return system
