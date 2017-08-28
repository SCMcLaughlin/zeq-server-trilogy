
--------------------------------------------------------------------------------
-- Imports
--------------------------------------------------------------------------------
local eChatChannel = require "enum/chat_channel"
local eChatColor = require "enum/chat_color"
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
-- Caches
--------------------------------------------------------------------------------
local lower = string.lower
local tonumber = tonumber
--------------------------------------------------------------------------------

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

    level = function(e)
        local lvl = tonumber(e.args[1])
        local targ = e.self:getTargetOrSelf()
        
        if lvl then
            targ:updateLevel(lvl)
        else
            e.self:message(eChatColor.Default, "%s's level: %u", targ:getName(), targ:getLevel())
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
    
    updatehp = function(e)
        local hp = tonumber(e.args[1])
        local targ = e.self:getTargetOrSelf()
        
        targ:updateHP(hp)
    end,
    
    updatemana = function(e)
        local mana = tonumber(e.args[1])
        local targ = e.self:getTargetOrSelf()
        
        targ:updateMana(mana)
    end,
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
