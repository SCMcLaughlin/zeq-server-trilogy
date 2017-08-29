
--------------------------------------------------------------------------------
-- Imports
--------------------------------------------------------------------------------
local Class = require "sys/class"
local System = require "sys/system"
local ScriptObject = require "sys/ScriptObject"
local C = require "sys/Mob_cdef"
local ffi = require "ffi"

require "sys/packet_cdef"
require "sys/Zone_cdef"
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

function Mob:getZone()
    local ptr = C.mob_get_zone(self:toMobPtr())
    if ptr == nil then return end
    
    return getObject(C.zone_lua_index(ptr))
end

function Mob:getEntityId()
    return C.mob_entity_id(self:toMobPtr())
end

function Mob:getHP()
    return tonumber(C.mob_cur_hp(self:toMobPtr()))
end

function Mob:getHP64()
    return C.mob_cur_hp(self:toMobPtr())
end

function Mob:getMaxHP()
    return tonumber(C.mob_max_hp(self:toMobPtr()))
end

function Mob:getMaxHP64()
    return C.mob_max_hp(self:toMobPtr())
end

function Mob:getMana()
    return tonumber(C.mob_cur_mana(self:toMobPtr()))
end

function Mob:getMana64()
    return C.mob_cur_mana(self:toMobPtr())
end

function Mob:getMaxMana()
    return tonumber(C.mob_max_mana(self:toMobPtr()))
end

function Mob:getMaxMana64()
    return C.mob_max_mana(self:toMobPtr())
end

function Mob:getLevel()
    return C.mob_level(self:toMobPtr())
end

function Mob:getClassId()
    return C.mob_class_id(self:toMobPtr())
end

function Mob:getGenderId()
    return C.mob_gender_id(self:toMobPtr())
end

function Mob:updateGenderId(genderId)
    C.mob_update_gender_id(self:toMobPtr(), genderId)
end

function Mob:getRaceId()
    return C.mob_race_id(self:toMobPtr())
end

function Mob:updateRaceId(raceId)
    C.mob_update_race_id(self:toMobPtr(), raceId)
end

function Mob:X()
    return C.mob_x(self:toMobPtr())
end

function Mob:Y()
    return C.mob_y(self:toMobPtr())
end

function Mob:Z()
    return C.mob_z(self:toMobPtr())
end

Mob.getX = Mob.X
Mob.getY = Mob.Y
Mob.getZ = Mob.Z

function Mob:getSize()
    return C.mob_cur_size(self:toMobPtr())
end

function Mob:getTextureId()
    return C.mob_texture_id(self:toMobPtr())
end

function Mob:updateTextureId(texId)
    C.mob_update_texture_id(self:toMobPtr(), texId)
end

function Mob:getHelmTextureId()
    return C.mob_helm_texture_id(self:toMobPtr())
end

function Mob:updateHelmTextureId(texId)
    C.mob_update_helm_texture_id(self:toMobPtr(), texId)
end

function Mob:getMaterialId(n)
    return C.mob_material_id(self:toMobPtr(), n)
end

function Mob:updateMaterialId(n, matId)
    C.mob_update_material_id(self:toMobPtr(), n, matId)
end

function Mob:getTint(n)
    return C.mob_tint(self:toMobPtr(), n)
end

function Mob:updateTint(n, color)
    C.mob_update_tint(self:toMobPtr(), n, color)
end

function Mob:updateLevel(lvl)
    C.mob_update_level(self:toMobPtr(), lvl)
end

function Mob:updateSize(size)
    C.mob_update_size(self:toMobPtr(), size)
end

function Mob:animate(animId)
    C.mob_animate_nearby(self:toMobPtr(), animId)
end

function Mob:animateRangeOverride(animId, range)
    C.mob_animate_range(self:toMobPtr(), animId, range)
end

function Mob:packetBroadcastNearby(packet, range)
    local zone = self:getZone()
    if not zone then return end
    
    zone:packetBroadcastAroundPoint(packet, self:X(), self:Y(), self:Z(), range)
end

function Mob:packetBroadcastNearbyExceptSelf(packet, range)
    local zone = self:getZone()
    if not zone then return end
    
    zone:packetBroadcastAroundPoint(packet, self:X(), self:Y(), self:Z(), range, self)
end

function Mob:castFx(spellId, castTimeMs)
    if not self:isValid() then return end
    
    local packet = C.packet_create_spell_cast_begin(self:toMobPtr(), spellId, castTimeMs or 5000)
    self:packetBroadcastNearby(packet)
end

return Mob
