
--------------------------------------------------------------------------------
-- Imports
--------------------------------------------------------------------------------
local Class = require "sys/class"
local Script = require "sys/script"
local ScriptObject = require "sys/ScriptObject"
local C = require "sys/Zone_cdef"
local ffi = require "ffi"

require "sys/log_cdef"
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
-- Caches
--------------------------------------------------------------------------------
local pairs = pairs
local format = string.format
local toLuaString = ffi.string
--------------------------------------------------------------------------------

local Zone = Class("Zone", ScriptObject)

function Zone.new(ptr)
    local zone = Zone:wrap(ptr)
    local env = zone:getPersonalEnvironment()
    local shortname = zone:getShortName()
    
    -- Create log function for this zone instance
    local logQueue = C.zone_log_queue(ptr)
    local logId = C.zone_log_id(ptr)
    
    env.log = function(self, fmt, ...)
        local str = format(fmt, ...)
        if str and #str > 0 then
            C.log_write(logQueue, logId, str, #str)
        end
    end
    
    -- Run scripts
    local genv = Script.run("script/global/global_zone.lua", zone)
    
    local path = "script/zone/".. shortname .."/zone.lua"
    local zenv = Script.run(path, zone)
    
    -- Merge environments
    for k, v in pairs(genv) do
        env[k] = v
    end
    
    for k, v in pairs(zenv) do
        env[k] = v
    end
    
    return zone
end

--------------------------------------------------------------------------------
-- Lookup optimization
--------------------------------------------------------------------------------
Zone.ptr = ScriptObject.ptr

function Zone:getClassName()
    return "Zone"
end

function Zone:getShortName()
    return toLuaString(C.zone_short_name(self:ptr()))
end

function Zone:getLongName()
    return toLuaString(C.zone_long_name(self:ptr()))
end

function Zone:packetBroadcastToAll(packet, except)
    C.zone_broadcast_to_all_clients_except(self:ptr(), packet, except and except:ptr() or nil)
end

function Zone:packetBroadcastAroundPoint(packet, x, y, z, range, except)
    C.zone_broadcast_to_nearby_clients_except(self:ptr(), packet, x, y, z, range or 200, except and except:ptr() or nil)
end

function Zone:timer(periodMs, func)
    return ScriptObject.timer(self, self, periodMs, func)
end

return Zone
