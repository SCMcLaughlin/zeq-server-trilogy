
local bit = require "bit"
local e = require "script/enum/slot_id"

local function slot(id)
    return bit.lshift(1, id)
end

local ears = bit.bor(slot(e.EAR1), slot(e.EAR2))
local head = slot(e.HEAD)
local face = slot(e.FACE)
local neck = slot(e.NECK)
local shoulders = slot(e.SHOULDERS)
local arms = slot(e.ARMS)
local back = slot(e.BACK)
local wrist = bit.bor(slot(e.WRIST1), slot(e.WRIST2))
local range = slot(e.RANGE)
local hands = slot(e.HANDS)
local primary = slot(e.PRIMARY)
local secondary = slot(e.SECONDARY)
local fingers = bit.bor(slot(e.FINGERS1), slot(e.FINGERS2))
local chest = slot(e.CHEST)
local legs = slot(e.LEGS)
local feet = slot(e.FEET)
local waist = slot(e.WAIST)
local ammo = slot(e.AMMO)

return {
    EARS = ears,
    EAR = ears,
    HEAD = head,
    FACE = face,
    NECK = neck,
    SHOULDERS = shoulders,
    SHOULDER = shoulders,
    ARMS = arms,
    ARM = arms,
    BACK = back,
    WRIST = wrist,
    WRISTS = wrist,
    RANGE = range,
    RANGED = range,
    HANDS = hands,
    HAND = hands,
    PRIMARY = primary,
    SECONDARY = secondary,
    FINGERS = fingers,
    FINGER = fingers,
    CHEST = chest,
    LEGS = legs,
    LEG = legs,
    FEET = feet,
    FOOT = feet,
    WAIST = waist,
    AMMO = ammo,
    ALL = bit.bor(ears, head, face, neck, shoulders, arms, back, wrist, range, hands, primary, secondary, fingers, chest, legs, feet, waist, ammo),
}
