
local convert = require "sys/load_item_convert"
local C = require "sys/log_cdef"

local assert = assert
local type = type
local tonumber = tonumber
local open = io.open
local gSkipPaths = gSkipPaths
local gLogQueue = gLogQueue
local gLogId = gLogId

local function warning(path, msg)
    local str = "WARNING: ".. path ..": ".. msg
    C.log_write(gLogQueue, gLogId, str, #str)
end

local validTags = {
    loreitem = true,
    nodrop = true,
    norent = true,
    magicitem = true,
}

local ignoreNumeric = {
    skill = true,
}

local validNumeric = {
    ac = true,
    dmg = true,
    damage = true,
    dly = true,
    delay = true,
    atkdly = true,
    atkdelay = true,
    str = true,
    sta = true,
    cha = true,
    dex = true,
    agi = true,
    int = true,
    wis = true,
    svmagic = true,
    svm = true,
    svfire = true,
    svf = true,
    svcold = true,
    svc = true,
    svpoison = true,
    svp = true,
    svdisease = true,
    svd = true,
    wt = true,
    model = true,
    icon = true,
    haste = true,
}

local translateNumeric = {
    damage = "dmg",
    delay = "dly",
    atkdly = "dly",
    atkdelay = "dly",
    svm = "svmagic",
    svf = "svfire",
    svc = "svcold",
    svp = "svpoison",
    svd = "svdisease",
}

local validString = {
    skill = true,
    type = true,
    slot = true,
    slots = true,
    race = true,
    races = true,
    class = true,
    classes = true,
    size = true,
}

local translateString = {
    type = "skill",
    slots = "slot",
    races = "race",
    classes = "class",
}

local function ItemDef(path, shortpath, itemId, defstr)
    local item = {}
    
    -- Name
    item.name = defstr:match("%s*([^\n]+)")
    
    if not item.name then
        warning("item has no name")
    end
    
    local tagline = defstr:match("\n([^\n+])")
    
    -- Tags
    for tag in tagline:gmatch("(%u+ %u+)") do
        local ttag = tag:lower()
        ttag = ttag:gsub("%s+", "")
        
        if validTags[ttag] then
            item[ttag] = 1
        else
            warning(path, "unknown tag ".. tag)
        end
    end
    
    -- Numeric stats
    for key, val in defstr:gmatch("(%a[^\n:]-):[ \t]*([%+%-]?%d+%.?%d*)%%?") do
        local tkey = key:lower()
        tkey = tkey:gsub("%s+", "")
        
        if ignoreNumeric[tkey] then goto skip_num end
        
        if validNumeric[tkey] then
            item[translateNumeric[tkey] or tkey] = tonumber(val)
        else
            warning(path, "unknown numeric field ".. key .." with value ".. val)
        end
        
        ::skip_num::
    end
    
    -- String settings
    for key, val in defstr:gmatch("(%a[^\n:]-):[ \t]*([12]?%a[^\n]+)") do
        local tkey = key:lower()
        tkey = tkey:gsub("%s+", "")
        
        if validString[tkey] then
            item[translateString[tkey] or tkey] = val
        else
            warning(path, "unknown string field ".. key .." with value ".. val)
        end
    end
    
    return convert(path, shortpath, itemId, item)
end

return function(path)
    local shortpath = path:match("^script/item/(.+)%.lua$")
    local found = gSkipPaths[shortpath]
    local itemId = 0
    
    if found then
        if type(found) == "boolean" then return end
        
        itemId = found
    end
    
    local file = assert(open(path, "rb"))
    local str = file:read("*a")
    file:close()
    
    local defstr = str:match("ItemDef%[%[(.-)%]%]")
    
    if defstr then
        return ItemDef(path, shortpath, itemId, defstr)
    end
end
