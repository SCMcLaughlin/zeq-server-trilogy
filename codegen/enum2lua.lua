
local lfs = require "lfs"

local tonumber = tonumber

local function modtime(path)
    return lfs.attributes(path, "modification") or 0
end

local function enum2lua(name, prefix)
    local srcname = "src/".. name ..".h"
    local dstname = "bin/script/enum/".. name ..".lua"
    
    if modtime(srcname) <= modtime(dstname) then return end
    
    local file = assert(io.open(srcname, "rb"))
    local str = file:read("*a")
    file:close()
    
    str = str:gsub("#[^\n]+", "")
    str = str:gsub("//[^\n]+", "")
    str = str:gsub("/%*.-%*/", "")
    
    local out = {}
    
    local value = 0
    for key, val in str:gmatch(prefix .."([%w_]+)%s*=?%s*(%w*)") do
        if val ~= "" then
            value = tonumber(val)
        end
        
        out[#out + 1] = '    ["' .. key .. '"] = '.. value ..','
        value = value + 1
    end
    
    file = assert(io.open(dstname, "wb+"))
    file:write("\nreturn {\n", table.concat(out, "\n"), "\n}\n")
    file:close()
end

enum2lua("item_stat_id", "ITEM_STAT_")
enum2lua("item_type_id", "ITEM_TYPE_")
enum2lua("chat_color", "CHAT_COLOR_")
enum2lua("chat_channel", "CHAT_CHANNEL_")
