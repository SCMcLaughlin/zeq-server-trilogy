
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

local handers = {
    castfx = function(e)
        local spellId = tonumber(e.args[1]) or 3
        local castTimeMs = tonumber(e.args[2])
        
        e.self:castFx(spellId, castTimeMs)
    end,
}

return function(e)
    local cmd = lower(e.cmd)
    local func = handlers[cmd]
    
    e.self:getZone():log(e.cmd)
    
    if func then
        func(e)
    else
        e.self:message(eChatColor.Red, "Unknown command: %s", e.cmd)
    end
end
