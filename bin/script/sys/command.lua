
--------------------------------------------------------------------------------
-- Imports
--------------------------------------------------------------------------------
local eChatChannel = require "enum/chat_channel"
local eChatColor = require "enum/chat_color"
local bit = require "bit"
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
-- Caches
--------------------------------------------------------------------------------
local lower = string.lower
local tonumber = tonumber
local math = math
--------------------------------------------------------------------------------

local function material(e)
    local slotId = tonumber(e.args[1])
    local matId = tonumber(e.args[2])
    local targ = e.self:getTargetOrSelf()

    if slotId and matId then
        targ:updateMaterialId(slotId, matId)
    else
        e.self:message(eChatColor.Red, "Usage: %s [slotId] [materialId]", lower(e.cmd))
    end
end

local handlers = {
    anim = function(e)
        local animId = tonumber(e.args[1])
        local targ = e.self:getTargetOrSelf()
        
        if animId then
            targ:animate(animId)
        else
            e.self:message(eChatColor.Red, "Usage: anim [animId]")
        end
    end,
    
    b = function(e)
        local spellId = tonumber(e.args[1]) or 0xffffffff
        local offset = tonumber(e.args[2]) or 0
        local value = tonumber(e.args[3]) or 1
        
        local C = require "sys/packet_cdef"
        require "sys/Client_cdef"
        
        local p = C.packet_create_test(e.self:getEntityId(), spellId, offset, value)
        C.client_schedule_packet(e.self:ptr(), p)
    end,
    
    castfx = function(e)
        local spellId = tonumber(e.args[1]) or 3
        local castTimeMs = tonumber(e.args[2])
        local targ = e.self:getTargetOrSelf()
        
        targ:castFx(spellId, castTimeMs)
    end,
    
    colortest = function(e)
        local colorId = tonumber(e.args[1])
        local self = e.self
        
        if colorId then
            self:message(colorId, "colortest")
        else
            self:message(eChatColor.Red, "Usage: colortest [colorId]")
        end
    end,
    
    entityid = function(e)
        local targ = e.self:getTargetOrSelf()
        e.self:message(eChatColor.Default, "%s's entityId: %i", targ:getName(), targ:getEntityId())
    end,

    exp = function(e)
        local exp = tonumber(e.args[1])
        local targ = e.self:getTargetOrSelf()

        if not targ:isClient() then
            e.self:message(eChatColor.Default, "NPCs don't have exp!")
        elseif exp then
            targ:updateExp(exp)
        else
            e.self:message(eChatColor.Default, "%s's exp: %u", targ:getName(), targ:getExperience())
        end
    end,
    
    gender = function(e)
        local id = tonumber(e.args[1])
        local targ = e.self:getTargetOrSelf()
        
        if id then
            targ:updateGenderId(id)
        else
            e.self:message(eChatColor.Default, "%s's gender: %u", targ:getName(), targ:getGenderId())
        end
    end,
    
    helmtexture = function(e)
        local id = tonumber(e.args[1])
        local targ = e.self:getTargetOrSelf()
        
        if id then
            targ:updateHelmTextureId(id)
        else
            e.self:message(eChatColor.Default, "%s's helmtexture: %u", targ:getName(), targ:getHelmTextureId())
        end
    end,
    
    hp = function(e)
        local hp = tonumber(e.args[1])
        local targ = e.self:getTargetOrSelf()
        
        if hp then
            targ:updateHP(hp)
        else
            e.self:message(eChatColor.Default, "%s's hp: %i / %i", targ:getName(), targ:getHP(), targ:getMaxHP())
        end
    end,

    level = function(e)
        local lvl = tonumber(e.args[1])
        local targ = e.self:getTargetOrSelf()
        
        if lvl then
            targ:updateLevel(lvl)
        else
            e.self:message(eChatColor.Default, "%s's level: %u", targ:getName(), targ:getLevel())
        end
    end,

    m = material,    

    mana = function(e)
        local mana = tonumber(e.args[1])
        local targ = e.self:getTargetOrSelf()
        
        if mana then
            targ:updateMana(mana)
        else
            e.self:message(eChatColor.Default, "%s's mana: %i / %i", targ:getName(), targ:getMana(), targ:getMaxMana())
        end
    end,

    material = material,
    
    race = function(e)
        local id = tonumber(e.args[1])
        local targ = e.self:getTargetOrSelf()
        
        if id then
            targ:updateRaceId(id)
        else
            e.self:message(eChatColor.Default, "%s's race: %u", targ:getName(), targ:getRaceId())
        end
    end,
   
    size = function(e)
        local size = tonumber(e.args[1])
        local targ = e.self:getTargetOrSelf()
        
        if size then
            targ:updateSize(size)
        else
            e.self:message(eChatColor.Default, "%s's size: %g", targ:getName(), targ:getSize())
        end
    end,
    
    spawnappearance = function(e)
        local type = tonumber(e.args[1])
        local value = tonumber(e.args[2])
        local targ = e.self:getTargetOrSelf()
        
        local C = require "sys/packet_cdef"
        require "sys/Client_cdef"
        
        local p = C.packet_create_spawn_appearance(e.self:getEntityId(), type or 3, value or 0)
        C.client_schedule_packet(e.self:ptr(), p)
    end,
    
    texture = function(e)
        local id = tonumber(e.args[1])
        local targ = e.self:getTargetOrSelf()
        
        if id then
            targ:updateTextureId(id)
        else
            e.self:message(eChatColor.Default, "%s's texture: %u", targ:getName(), targ:getTextureId())
        end
    end,

    tint = function(e)
        local slotId = tonumber(e.args[1])
        local red = tonumber(e.args[2]) or 0
        local green = tonumber(e.args[3]) or 0
        local blue = tonumber(e.args[4]) or 0
        local targ = e.self:getTargetOrSelf()

        if not slotId then
            e.self:message(eChatColor.Red, "Usage: tint [red] [green] [blue]")
            return
        end
        
        red = math.floor(math.ceil(red, 255), 0)
        green = math.floor(math.ceil(green, 255), 0)
        blue = math.floor(math.ceil(blue, 255), 0)        

        local color = bit.bor(bit.lshift(red, 24), bit.lshift(green, 16), bit.lshift(blue, 8))
        targ:updateTint(slotId, color)
    end,

    wc = material,
}

return function(e)
    local cmd = lower(e.cmd)
    local func = handlers[cmd]
    
    if func then
        func(e)
    else
        e.self:message(eChatColor.Red, "Unknown command: %s", e.cmd)
    end
end
