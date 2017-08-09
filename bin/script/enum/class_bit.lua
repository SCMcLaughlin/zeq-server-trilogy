
local bit = require "bit"
local e = require "enum/class_id"

local function class(id)
    return bit.lshift(1, id - 1)
end

local war = class(e.WAR)
local clr = class(e.CLR)
local pal = class(e.PAL)
local rng = class(e.RNG)
local shd = class(e.SHD)
local dru = class(e.DRU)
local mnk = class(e.MNK)
local rog = class(e.ROG)
local brd = class(e.BRD)
local shm = class(e.SHM)
local nec = class(e.NEC)
local wiz = class(e.WIZ)
local mag = class(e.MAG)
local enc = class(e.ENC)

return {
    WAR = war,
    CLR = clr,
    CLE = clr,
    PAL = pal,
    RNG = rng,
    SHD = shd,
    SK = shd,
    DRU = dru,
    MNK = mnk,
    ROG = rog,
    BRD = brd,
    SHM = shm,
    NEC = nec,
    WIZ = wiz,
    MAG = mag,
    ENC = enc,
    ALL = bit.bor(war, clr, pal, rng, shd, dru, mnk, rog, brd, shm, nec, wiz, mag, enc),
}
