
local lfs = require "lfs"
local parse = require "sys/load_item_parse"

local dir = lfs.dir
local attributes = lfs.attributes
local gLoadedCount = 0

local function isDir(path)
    return attributes(path, "mode") == "directory"
end

local function iterate(path)
    for name in dir(path) do
        if name == "." or name == ".." then goto skip end
        
        if not name:find("%.lua$") then goto skip end
        
        local namepath = path .."/".. name
        
        if isDir(namepath) then
            iterate(namepath)
        else
            if parse(namepath) then
                gLoadedCount = gLoadedCount + 1
            end
        end
        
        ::skip::
    end
end

iterate("script/item")

return gLoadedCount
