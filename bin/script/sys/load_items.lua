
local lfs = require "lfs"
local bit = require "bit"
local eSlot = require "script/enum/slot_bit"
local eClass = require "script/enum/class_bit"
local eRace = require "script/enum/race_bit"

local tonumber = tonumber
local pairs = pairs
local floor = math.floor
local bor = bit.bor
local lshift = bit.lshift

local warnPath
local function warning(msg)
    io.write("WARNING: ".. warnPath ..": ".. msg .."\n")
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
            print(class, v)
            
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
            print(race, v)
            
            ::skip::
        end
        
        return v
    end,
    
    wt = function(num)
        return floor(num * 10)
    end,
}

local function makeItem(fields)
    -- Process fields that require it
    local item = {}
    
    for k, v in pairs(fields) do
        local func = processField[k]
        
        if func then
            item[k] = func(v)
        else
            item[k] = v
        end
    end
    
    -- Create the ItemProto on the C side
    --fixme: implement this
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

local function ItemDef(path, defstr)
    local item = {}
    
    warnPath = path
    
    -- Name
    item.name = defstr:match("%s*[^\n]+")
    
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
        
        io.write(tkey, ": ", tonumber(val), "\n")
        
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
        
        io.write(tkey, ": ", val, "\n")
        
        if validString[tkey] then
            item[translateString[tkey] or tkey] = val
        else
            warning("unknown string field ".. key .." with value ".. val)
        end
    end
    
    makeItem(item)
end

local function parse(path)
    local file = assert(io.open(path, "rb"))
    local str = file:read("*a")
    file:close()
    
    local defstr = str:match("ItemDef%[%[(.-)%]%]")
    
    if defstr then
        ItemDef(path, defstr)
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
