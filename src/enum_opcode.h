
#ifndef ENUM_OPCODE_H
#define ENUM_OPCODE_H

enum Opcode
{
    /* Zone entry */
    OP_SetDataRate          = 0x21e8,
    OP_ZoneEntry            = 0x202a,
    OP_PlayerProfile        = 0x202d,
    OP_Weather              = 0x2136,
    OP_InventoryRequest     = 0x205d,
    OP_ZoneInfoRequest      = 0x200a,
    OP_ZoneInfo             = 0x205b,
    OP_ZoneInUnknown        = 0x2147,
    OP_EnterZone            = 0x20d8,
    OP_EnteredZoneUnknown   = 0x21c3,
    
    OP_Inventory            = 0x2031,
    OP_PositionUpdate       = 0x20a1,
    OP_ClientPositionUpdate = 0x20f3,
    OP_SpawnAppearance      = 0x20f5,
    OP_Spawn                = 0x2149,
    OP_SpawnsCompressed     = 0x2161,
    OP_Unspawn              = 0x202b,
    OP_CampStart            = 0x2207,
    OP_Save                 = 0x202e,
    
    OP_CustomMessage        = 0x2180,
    OP_Message              = 0x2107,
    
    OP_SwapItem             = 0x212c,
    OP_MoveCoin             = 0x212d,
    
    OP_SpellCastBegin       = 0x20a9,   /* Spell casting particles */
    OP_SpellCastFinish      = 0x2046,   /* Apply buff and particles on the target */
    OP_ClientSpellCast      = 0x217e,
    OP_InterruptSpell       = 0x21d3,
    OP_Buff                 = 0x2132,
    OP_SpellState           = 0x2182,
    OP_SwapSpell            = 0x21ce,

    OP_HpUpdate             = 0x20b2,
    OP_ManaUpdate           = 0x217f,
    OP_LevelUpdate          = 0x2198,
    OP_ExpUpdate            = 0x2199,
    OP_AppearanceUpdate     = 0x2091,
    
    OP_Animation            = 0x209f,

    OP_Target               = 0x2162,
};

#endif/*ENUM_OPCODE_H*/
