
--------------------------------------------------------------------------------
-- Imports
--------------------------------------------------------------------------------
local ZT = require "ZoneThread"
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
-- Caches
--------------------------------------------------------------------------------
local rawset = rawset
local rawget = rawget
local xpcall = xpcall
local loadfile = loadfile
local setfenv = setfenv
local setmetatable = setmetatable
local traceback = debug.traceback
--------------------------------------------------------------------------------

local script = {}
local mt = {__index = _G}

local function resetEnv(env)
    local inherit = rawget(env, "__index")
    for k in pairs(env) do
        env[k] = nil
    end
    rawset(env, "__index", inherit)
end

function script.run(path, logger)
    logger = logger or ZT
    
    local env = setmetatable({}, mt)
    local script = loadfile(path)
    if script then
        setfenv(script, env)
        local s, err = xpcall(script, traceback)
        if not s then
            logger:log(err)
            resetEnv(env) -- Loading a script is all-or-nothing, ensure the env is not half-written
        end
    end
    return env
end

return script
