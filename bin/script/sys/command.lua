
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
    castfx = function(e)
        local spellId = tonumber(e.args[1]) or 3
        local castTimeMs = tonumber(e.args[2])
        
        e.self:castFx(spellId, castTimeMs)
    end,
    
    colortest = function(e)
        local colorId = tonumber(e.args[1])
        local self = e.self
        
        if colorId then
            self:message(colorId, "colortest")
        else
            self:message(eChatColor.Red, "Usage: colortext [colorId]")
        end
    end,
    
    size = function(e)
        local size = tonumber(e.args[1])
        local targ = e.self:getTargetOrSelf()
        if not targ then return end
        
        if size then
            targ:updateSize(size)
        else
            e.self:message(eChatColor.Default, "%s's size: %g", targ:getName(), targ:getSize())
        end
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
