
#ifndef ENUM_CHAR_SELECT_OPCODE_H
#define ENUM_CHAR_SELECT_OPCODE_H

enum CharSelectOpcode
{
    /* Login handshake, in order of occurrence */
    OP_CS_LoginInfo         = 0x1858,
    OP_CS_LoginApproved     = 0x1007,
    OP_CS_Enter             = 0x8001,
    OP_CS_ExpansionInfo     = 0x21d8,
    OP_CS_CharacterInfo     = 0x2047,
    OP_CS_GuildList         = 0x2192,
    /* Requests from the client */
    OP_CS_NameApproval      = 0x208b,
    OP_CS_CreateCharacter   = 0x2049,
    OP_CS_DeleteCharacter   = 0x205a,
    OP_CS_WearChange        = 0x2092,
    /* Replies to client zone-in requests */
    OP_CS_ZoneUnavailable   = 0x8005,
    OP_CS_MessageOfTheDay   = 0x21dd,
    OP_CS_TimeOfDay         = 0x20f2,
    OP_CS_ZoneAddress       = 0x8004,
    /* Packets that need to have their opcode echoed in a zero-byte packet for whatever reason */
    OP_CS_Echo1             = 0x2023,
    OP_CS_Echo2             = 0x80a9,
    OP_CS_Echo3             = 0xab00,
    OP_CS_Echo4             = 0xac00,
    OP_CS_Echo5             = 0xad00,
    /* Packets the client sends that we have no interest in */
    OP_CS_Ignore1           = 0x2139,
};

#endif/*ENUM_CHAR_SELECT_OPCODE_H*/
