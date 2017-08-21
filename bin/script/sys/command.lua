
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
--------------------------------------------------------------------------------

local handers = {
    castfx = function(e)
        local spellId = e.args[1] or 3
        local castTimeMs = e.args[2]
        
        e.self:castFx(spellId, castTimeMs)
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
