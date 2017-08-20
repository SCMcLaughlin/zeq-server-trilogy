
--------------------------------------------------------------------------------
-- Imports
--------------------------------------------------------------------------------
local Class = require "sys/class"
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
-- Caches
--------------------------------------------------------------------------------
local getmetatable = getmetatable
local error = error
--------------------------------------------------------------------------------

local ScriptObject = Class("ScriptObject")

function ScriptObject:ptr()
    local ptr = self._ptr
    if ptr == nil then
        error "Attempt to use an object that has been destroyed on the C side"
    end
    return ptr
end

function ScriptObject:isValid()
    return self._ptr ~= nil
end

--------------------------------------------------------------------------------
-- exists() is an alias for isValid()
--------------------------------------------------------------------------------
ScriptObject.exists = ScriptObject.isValid

function ScriptObject:getClassName()
    return "ScriptObject"
end

function ScriptObject:getPersonalEnvironment()
    return getmetatable(self)
end

return ScriptObject
