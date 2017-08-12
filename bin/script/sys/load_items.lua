
local lfs = require "lfs"
local bit = require "bit"
local eSlot = require "enum/slot_bit"
local eClass = require "enum/class_bit"
local eRace = require "enum/race_bit"
local C = require "sys/load_items_cdef"
require "sys/log_cdef"

local tonumber = tonumber
local pairs = pairs
local type = type
local floor = math.floor
local bor = bit.bor
local lshift = bit.lshift

local gSkipPaths = gSkipPaths
local gItemList = gItemList
local gChanges = gChanges
local gLogQueue = gLogQueue
local gLogId = gLogId
local gLoadedCount = 0

local warnPath
local function warning(msg)
    local str = "WARNING: ".. warnPath ..": ".. msg
    C.log_write(gLogQueue, gLogId, str, #str)
end

local function throw(msg)
    error("ERROR: ".. warnPath ..": ".. msg)
end

local processField = {
    slot = function(str)
        local v = 0
        
        for slot in str:gmatch("%a+") do
            local bit = eSlot[slot]
            
            if not bit then
                warning("unknown slot: ".. slot)
                goto skip
            end
            
            v = bor(v, bit)
            
            ::skip::
        end
        
        return v
    end,
    
    class = function(str)
        local v = 0
        
        for class in str:gmatch("%a+") do
            local bit = eClass[class]
            
            if not bit then
                warning("unknown class: ".. class)
                goto skip
            end
            
            v = bor(v, bit)
            
            ::skip::
        end
        
        return v
    end,
    
    race = function(str)
        local v = 0
        
        for race in str:gmatch("%a+") do
            local bit = eRace[race]
            
            if not bit then
                warning("unknown race: ".. race)
                goto skip
            end
            
            v = bor(v, bit)
            
            ::skip::
        end
        
        return v
    end,
    
    wt = function(num)
        return floor(num * 10)
    end,
}

local function take(tbl, key, count)
    local val = tbl[key]
    tbl[key] = nil
    return val, val == nil and count or count - 1
end

local function makeItem(path, shortpath, itemId, fields)
    -- Process fields that require it
    local item = {}
    local count = 0
    
    for k, v in pairs(fields) do
        local func = processField[k]
        
        if func then
            item[k] = func(v)
        else
            item[k] = v
        end
        
        count = count + 1
    end
    
    -- Create the ItemProto on the C side
    local name, lore, slot
    
    name, count = take(item, "name", count)
    lore, count = take(item, "lore", count)
    
    local proto = C.item_proto_add(gItemList, gChanges, lfs.attributes(path, "modification"), itemId, shortpath, #shortpath, count)
    
    if proto == nil then
        throw("item_proto_add() failed")
    end
    
    if name and not C.item_proto_set_name(proto, name, #name) then
        throw("item_proto_set_name() failed")
    end
    
    if lore and not C.item_proto_set_lore(proto, lore, #lore) then
        throw("item_proto_set_lore() failed")
    end
    
    --fixme: translate field names to ids and set them here
    
    gLoadedCount = gLoadedCount + 1
end

local validTags = {
    loreitem = true,
    nodrop = true,
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
    
    warnPath = path
    
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
            warning("unknown tag ".. tag)
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
            warning("unknown numeric field ".. key .." with value ".. val)
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
            warning("unknown string field ".. key .." with value ".. val)
        end
    end
    
    makeItem(path, shortpath, itemId, item)
end

local function parse(path)
    local shortpath = path:match("^script/item/(.+)%.lua$")
    local found = gSkipPaths[shortpath]
    local itemId = 0
    
    if found then
        if type(found) == "boolean" then return end
        
        itemId = found
    end
    
    local file = assert(io.open(path, "rb"))
    local str = file:read("*a")
    file:close()
    
    local defstr = str:match("ItemDef%[%[(.-)%]%]")
    
    if defstr then
        ItemDef(path, shortpath, itemId, defstr)
    end
end

local function isDir(path)
    return lfs.attributes(path, "mode") == "directory"
end

local function iterate(path)
    for name in lfs.dir(path) do
        if name == "." or name == ".." then goto skip end
        
        local namepath = path .."/".. name
        
        if isDir(namepath) then
            iterate(namepath)
        else
            parse(namepath)
        end
        
        ::skip::
    end
end

iterate("script/item")

return gLoadedCount
