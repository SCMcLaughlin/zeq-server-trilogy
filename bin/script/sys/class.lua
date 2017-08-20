
--------------------------------------------------------------------------------
-- Imports
--------------------------------------------------------------------------------
local util = require "util"
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
-- Caches
--------------------------------------------------------------------------------
local setmetatable = setmetatable
--------------------------------------------------------------------------------

local function call(t, ...)
    return t.new(...)
end

local Class = {
    __call = call,
    __index = _G,
}

setmetatable(Class, Class)

--------------------------------------------------------------------------------
-- Creates a new class type with a name and optional single inheritance.
--------------------------------------------------------------------------------
function Class.new(name, superClass)
    local is = "is" .. name
    Class[is] = util.falseFunc
    
    local newClass = {
        __call = call,
        [is] = util.trueFunc,
    }
    
    newClass.__index = superClass or Class
    Class[name] = newClass
    
    return setmetatable(newClass, newClass)
end

--------------------------------------------------------------------------------
-- Simple, pure-lua classes can use this to turn a table into an instance of a
-- class.
--------------------------------------------------------------------------------
function Class.instance(class, tbl)
    tbl = tbl or {}
    tbl.__index = class
    return setmetatable(tbl, tbl)
end

--------------------------------------------------------------------------------
-- Classes that wrap complex C-side objects use this instead.
--
-- The inheritance heirarchy for these classes looks like this:
--
-- [object]
--   -> personal environment (a table specifically for this one object)
--     -> specific class for the object
--       -> Class (as defined in this file)
--
-- Any writes made to fields on an object are stored in that object's personal
-- environment table.
--
-- (Note for sake of sanity: each object/environment is its own metatable, and
--  its __index points to the next environment in the heirarchy.)
--------------------------------------------------------------------------------
function Class.wrap(class, ptr)
    local personalEnv = {__index = class}
    
    setmetatable(personalEnv, personalEnv)
    
    local obj = {
        _ptr = ptr,
        __index = personalEnv,
        __newindex = personalEnv,
    }
    
    return setmetatable(obj, obj)
end

return Class
