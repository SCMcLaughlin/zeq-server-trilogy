
local lfs = require "lfs"

local function modtime(path, dir)
    return lfs.attributes((dir or "src/") .. path, "modification") or 0
end

local baseTime = modtime("amalg-zeq-server-trilogy.c", "codegen/")
local needsUpdate = false

for filename in lfs.dir("src") do
    if filename:find("%.[hc]$") and modtime(filename) >= baseTime then
        needsUpdate = true
        break
    end
end

if not needsUpdate then return end

local included = {}
local dst = assert(io.open(arg[1], "w+"))

local function readFile(path)
    local file = assert(io.open("src/".. path, "rb"))
    local data = file:read("*a")
    file:close()
    return data
end

local function includeFile(path)
    if included[path] then return "" end
    
    included[path] = true
    
    local str = readFile(path)
    
    str = str:gsub("/%*.-%*/", "")
    str = str:gsub("//[^\n]+", "")
    str = str:gsub('#%s*include%s*"([^"]+)"[^\n]*\n', includeFile)
    
    return str
end

for i = 2, #arg do
    dst:write(includeFile(arg[i] .. ".c"))
end

dst:close()
