
--------------------------------------------------------------------------------
-- Imports
--------------------------------------------------------------------------------
local Class = require "sys/class"
local ScriptObject = require "sys/ScriptObject"
local ffi = require "ffi"
local C = require "sys/packet_cdef"
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
-- Caches
--------------------------------------------------------------------------------
local toLuaString = ffi.string
local format = string.format
--------------------------------------------------------------------------------

local Mob = Class("Mob", ScriptObject)

function Mob:castFx(spellId, castTimeMs)
    if not self:isValid() then return end
    
    local packet = C.packet_create_spell_cast_begin(self:toMobPtr(), spellId, castTimeMs or 5000)
    --fixme: broadcast to nearby clients
end

return Mob
