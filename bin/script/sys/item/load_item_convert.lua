
local bit = require "bit"
local eSlot = require "enum/slot_bit"
local eClass = require "enum/class_bit"
local eRace = require "enum/race_bit"
local eStat = require "enum/item_stat_id"
local eType = require "enum/item_type_id"
local C = require "sys/load_items_cdef"
require "sys/log_cdef"

local tostring = tostring
local pairs = pairs
local type = type
local floor = math.floor
local bor = bit.bor
local lshift = bit.lshift
local gItemList = gItemList
local gChanges = gChanges
local gLogQueue = gLogQueue
local gLogId = gLogId
local gPath

local function warning(msg)
    local str = "WARNING: ".. gPath ..": ".. msg
    C.log_write(gLogQueue, gLogId, str, #str)
end

local function throw(msg)
    error("ERROR: ".. gPath ..": ".. msg)
end

local sizeToNum = {
    tiny = 0,
    small = 1,
    medium = 2,
    large = 3,
    giant = 4,
}

local skillToType = {
    ["1hs"] = eType["1HSlash"],
    ["1hslash"] = eType["1HSlash"],
    ["1hslashing"] = eType["1HSlash"],
    ["2hs"] = eType["2HSlash"],
    ["2hslash"] = eType["2HSlash"],
    ["2hslashing"] = eType["2HSlash"],
    ["1hb"] = eType["1HBlunt"],
    ["1hblunt"] = eType["1HBlunt"],
    ["2hb"] = eType["2HBlunt"],
    ["2hblunt"] = eType["2HBlunt"],
    piercing = eType["1HPiercing"],
    pierce = eType["1HPiercing"],
    ["1hp"] = eType["1HPiercing"],
    ["1hpierce"] = eType["1HPiercing"],
    ["1hpiercing"] = eType["1HPiercing"],
    ["2hp"] = eType["2HPiercing"],
    ["2hpierce"] = eType["2HPiercing"],
    ["2hpiercing"] = eType["2HPiercing"],
    stackable = eType.Stackable,
    stacking = eType.Stackable,
    stack = eType.Stackable,
    bag = eType.Bag,
    container = eType.Bag,
    tradeskill = eType.TradeskillBag,
    combine = eType.TradeskillBag,
    food = eType.Food,
    drink = eType.Drink,
    book = eType.Book,
    scroll = eType.Scroll,
    spell = eType.Scroll,
    bow = eType.Archery,
    archery = eType.Archery,
    arrow = eType.Arrow,
    throw = eType.Throwing,
    throwing = eType.Throwing,
    throwable = eType.Throwing,
    throwingstackable = eType.ThrowingStackable,
    throwstackable = eType.ThrowingStackable,
    throwstacking = eType.ThrowingStackable,
    throwingstack = eType.ThrowingStackable,
    stackingthrowable = eType.ThrowingStackable,
    shield = eType.Shield,
    bash = eType.Shield,
}

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
    
    size = function(str)
        local v = sizeToNum[str:lower():match("%a+")]
        
        if not v then
            warning("unknown size: ".. str)
            return 2
        end
        
        return v
    end,
    
    skill = function(str)
        local skill = str:lower():gsub("%s+", "")
        local id = skillToType[skill]
        
        if not id then
            warning("unknown skill: ".. skill)
            return 1
        end
        
        return id
    end,
}

local fieldToId = {
    loreitem = eStat.Lore,
    nodrop = eStat.NoDrop,
    norent = eStat.NoRent,
    magicitem = eStat.Magic,
    ac = eStat.AC,
    dmg = eStat.DMG,
    dly = eStat.Delay,
    str = eStat.STR,
    sta = eStat.STA,
    cha = eStat.CHA,
    dex = eStat.DEX,
    agi = eStat.AGI,
    int = eStat.INT,
    wis = eStat.WIS,
    svmagic = eStat.SvMagic,
    svfire = eStat.SvFire,
    svcold = eStat.SvCold,
    svpoison = eStat.SvPoison,
    svdisease = eStat.SvDisease,
    wt = eStat.Weight,
    model = eStat.Model,
    icon = eStat.Icon,
    haste = eStat.Haste,
    skill = eStat.ItemType,
    slot = eStat.Slot,
    race = eStat.Race,
    class = eStat.Class,
    size = eStat.Size,
}

local function take(tbl, key, count)
    local val = tbl[key]
    tbl[key] = nil
    return val, val == nil and count or count - 1
end

return function(path, shortpath, itemId, fields)
    -- Process fields that require it
    local item = {}
    local count = 0
    
    gPath = path
    
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
    
    -- Translate field names to ids and set their values
    local index = 0
    for k, v in pairs(item) do
        local id = fieldToId[k]
        
        if not id then
            warning("unknown field: ".. k)
            id = eStat.NONE
        end
        
        if type(v) ~= "number" then
            warning("non-numeric value for field ".. k ..": ".. tostring(v))
            v = 0
        end
        
        if not C.item_proto_set_field(proto, index, id, v) then
            throw("item_proto_set_field() failed")
        end
        
        index = index + 1
    end
    
    return true
end
