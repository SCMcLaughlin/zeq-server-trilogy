
#ifndef ENUM_LOGIN_OPCODES_H
#define ENUM_LOGIN_OPCODES_H

enum LoginOpcode
{
    OP_LOGIN_Credentials                    = 0x0001,
    OP_LOGIN_Error                          = 0x0002,
    OP_LOGIN_Session                        = 0x0004,
    OP_LOGIN_Exit                           = 0x0005,
    OP_LOGIN_ServerList                     = 0x0046,
    OP_LOGIN_SessionKey                     = 0x0047,
    OP_LOGIN_ServerStatusRequest            = 0x0048,
    OP_LOGIN_ServerStatusAlreadyLoggedIn    = 0x0049, /* Rejects the client with a message saying they can only be logged in to one server at a time */
    OP_LOGIN_ServerStatusAccept             = 0x004a,
    OP_LOGIN_Banner                         = 0x0052,
    OP_LOGIN_Version                        = 0x0059,
};

#endif/*ENUM_LOGIN_OPCODES_H*/
