
local bit = require "bit"
local e = require "enum/race_id"

local function race(id)
    return bit.lshift(1, id - 1)
end

local hum = race(e.HUM)
local bar = race(e.BAR)
local eru = race(e.ERU)
local elf = race(e.ELF)
local hie = race(e.HIE)
local def = race(e.DEF)
local hef = race(e.HEF)
local dwf = race(e.DWF)
local trl = race(e.TRL)
local ogr = race(e.OGR)
local hfl = race(e.HFL)
local gnm = race(e.GNM)
local iks = race(e.GNM + 1) -- Special case

return {
    HUM = hum,
    BAR = bar,
    ERU = eru,
    ELF = elf,
    HIE = hie,
    DEF = def,
    HEF = hef,
    DWF = dwf,
    TRL = trl,
    OGR = ogr,
    HFL = hfl,
    GNM = gnm,
    IKS = iks,
    ALL = bit.bor(hum, bar, eru, elf, hie, def, hef, dwf, trl, ogr, hfl, gnm, iks),
}
