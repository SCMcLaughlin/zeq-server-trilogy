
--------------------------------------------------------------------------------
-- Imports
--------------------------------------------------------------------------------
local Class = require "sys/class"
local System = require "sys/system"
local ScriptObject = require "sys/ScriptObject"
local C = require "sys/Mob_cdef"
local ffi = require "ffi"

require "sys/packet_cdef"
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
-- Caches
--------------------------------------------------------------------------------
local toLuaString = ffi.string
local format = string.format
local getObject = System.getObject
--------------------------------------------------------------------------------

local Mob = Class("Mob", ScriptObject)

function Mob:getName()
    return toLuaString(C.mob_name_str(self:toMobPtr()))
end

function Mob:getTarget()
    local ptr = self:getTargetPtr()
    if ptr == nil then return end
    
    return getObject(C.mob_lua_index(ptr))
end

function Mob:getTargetPtr()
    return C.mob_target(self:toMobPtr())
end

function Mob:getTargetOrSelf()
    return self:getTarget() or self
end

function Mob:getSize()
    return C.mob_cur_size(self:toMobPtr())
end

function Mob:updateLevel(lvl)
    C.mob_update_level(self:toMobPtr(), lvl)
end

function Mob:updateSize(size)
    C.mob_update_size(self:toMobPtr(), size)
end

function Mob:castFx(spellId, castTimeMs)
    if not self:isValid() then return end
    
    local packet = C.packet_create_spell_cast_begin(self:toMobPtr(), spellId, castTimeMs or 5000)
    --fixme: broadcast to nearby clients
end

return Mob
