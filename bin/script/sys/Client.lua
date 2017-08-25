
--------------------------------------------------------------------------------
-- Imports
--------------------------------------------------------------------------------
local Class = require "sys/class"
local Mob = require "sys/Mob"
local Script = require "sys/script"
local ScriptObject = require "sys/ScriptObject"
local C = require "sys/Client_cdef"
local ffi = require "ffi"

require "sys/packet_cdef"
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
-- Caches
--------------------------------------------------------------------------------
local toLuaString = ffi.string
local format = string.format
local rawset = rawset
local tonumber = tonumber
--------------------------------------------------------------------------------

local Client = Class("Client", Mob)

function Client.new(ptr, zone)
    local client = Client:wrap(ptr)
    local env = client:getPersonalEnvironment()
    local shortname = zone:getShortName()
    
    rawset(client, "_zone", zone)
    
    -- Run scripts
    local genv = Script.run("script/global/global_client.lua", zone)
    
    local path = "script/zone/".. shortname .."/client.lua"
    local zenv = Script.run(path, zone)
    
    -- Merge environments
    for k, v in pairs(genv) do
        env[k] = v
    end
    
    for k, v in pairs(zenv) do
        env[k] = v
    end
    
    return client
end

--------------------------------------------------------------------------------
-- Lookup optimization
--------------------------------------------------------------------------------
Client.ptr = ScriptObject.ptr
Client.isValid = ScriptObject.isValid
Client.exists = ScriptObject.exists

function Client:getClassName()
    return "Client"
end

function Client:getZone()
    return self._zone
end

function Client:timer(periodMs, func)
    return ScriptObject.timer(self, self:getZone(), periodMs, func)
end

function Client:toMobPtr()
    return C.client_mob(self:ptr())
end

function Client:message(chatChannel, str, ...)
    if not self:isValid() then return end
    
    local msg = format(str, ...)
    
    if msg and #msg > 0 then
        local packet = C.packet_create_custom_message(chatChannel, msg, #msg)
        C.client_schedule_packet(self:ptr(), packet)
    end
end

function Client:getExperience()
    return tonumber(C.client_experience(self:ptr()))
end

function Client:updateLevel(lvl)
    C.client_update_level(self:ptr(), lvl)
end

function Client:updateExp(exp)
    C.client_update_exp(self:ptr(), exp)
end

--fixme: remove when the Mob one is corrected
function Client:castFx(spellId, castTimeMs)
    if not self:isValid() then return end
    
    local packet = C.packet_create_spell_cast_begin(self:toMobPtr(), spellId, castTimeMs or 5000)
    C.client_schedule_packet(self:ptr(), packet)
end

return Client
