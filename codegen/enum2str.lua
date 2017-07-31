
local lfs = require "lfs"

local function modtime(path)
    return lfs.attributes("src/" .. path, "modification") or 0
end

local baseTime = math.min(modtime("enum2str.h"), modtime("enum2str.c"))
local filenames = {}

for filename in lfs.dir("src") do
    if filename:find("^enum_.+%.h$") then
        filenames[#filenames + 1] = filename
    end
end

local needUpdate = false

for i = 1, #filenames do
    if modtime(filenames[i]) >= baseTime then
        needUpdate = true
        break
    end
end

if not needUpdate then return end

table.sort(filenames)

local funcProtos = {}
local funcImpls = {}
local includes = {}

for i = 1, #filenames do
    local filename = filenames[i]
    local file = assert(io.open("src/" .. filename, "rb"))
    local str = file:read("*a")
    file:close()
    
    local proto = "const char* enum2str_" .. filename:match("^enum_([%w_]+)%.h$") .. "(int e)"
    
    includes[#includes + 1] = '#include "' .. filename .. '"'
    funcProtos[#funcProtos + 1] = proto
    
    for enum in str:gmatch("%s+enum%s+[%w_%s]*{([^}]+)}[%w_%s]*;") do
        -- Remove any comments
        enum = enum:gsub("//[^\n]+", "")
        enum = enum:gsub("/%*.-%*/", "")
        
        local incValue = 0
        
        local func = {proto, "{", "    const char* ret;", "    switch (e)", "    {"}
        
        for key in enum:gmatch("([%w_]+)[^,]*") do
            func[#func + 1] = "    case " .. key .. ': ret = "' .. key .. '"; break;'
        end
        
        local i = #func
        
        func[i + 1] = '    default: ret = "UNKNOWN"; break;'
        func[i + 2] = "    }"
        func[i + 3] = "    return ret;"
        func[i + 4] = "}"
        
        funcImpls[#funcImpls + 1] = table.concat(func, "\n")
    end
end

local header = assert(io.open("src/enum2str.h", "wb+"))
header:write('\n#ifndef ENUM2STR_H\n#define ENUM2STR_H\n\n#include "define.h"\n\n', table.concat(funcProtos, ";\n"), ";\n\n#endif/*ENUM2STR_H*/\n")
header:close()

local code = assert(io.open("src/enum2str.c", "wb+"))
code:write('\n#include "enum2str.h"\n', table.concat(includes, "\n"), "\n\n", table.concat(funcImpls, "\n\n"), "\n")
code:close()
