
#include "enum2str.h"
#include "enum_login_server_rank.h"
#include "enum_account_status.h"
#include "enum_login_opcode.h"
#include "enum_zop.h"
#include "enum_char_select_opcode.h"
#include "enum_err.h"
#include "enum_login_server_status.h"

const char* enum2str_login_server_rank(int e)
{
    const char* ret;
    switch (e)
    {
    case LOGIN_SERVER_RANK_Standard: ret = "LOGIN_SERVER_RANK_Standard"; break;
    case LOGIN_SERVER_RANK_Preferred: ret = "LOGIN_SERVER_RANK_Preferred"; break;
    case LOGIN_SERVER_RANK_Legends: ret = "LOGIN_SERVER_RANK_Legends"; break;
    default: ret = "UNKNOWN"; break;
    }
    return ret;
}

const char* enum2str_account_status(int e)
{
    const char* ret;
    switch (e)
    {
    case ACCT_STATUS_Banned: ret = "ACCT_STATUS_Banned"; break;
    case ACCT_STATUS_Suspended: ret = "ACCT_STATUS_Suspended"; break;
    case ACCT_STATUS_Normal: ret = "ACCT_STATUS_Normal"; break;
    case ACCT_STATUS_GM: ret = "ACCT_STATUS_GM"; break;
    default: ret = "UNKNOWN"; break;
    }
    return ret;
}

const char* enum2str_login_opcode(int e)
{
    const char* ret;
    switch (e)
    {
    case OP_LOGIN_Credentials: ret = "OP_LOGIN_Credentials"; break;
    case OP_LOGIN_Error: ret = "OP_LOGIN_Error"; break;
    case OP_LOGIN_Session: ret = "OP_LOGIN_Session"; break;
    case OP_LOGIN_Exit: ret = "OP_LOGIN_Exit"; break;
    case OP_LOGIN_ServerList: ret = "OP_LOGIN_ServerList"; break;
    case OP_LOGIN_SessionKey: ret = "OP_LOGIN_SessionKey"; break;
    case OP_LOGIN_ServerStatusRequest: ret = "OP_LOGIN_ServerStatusRequest"; break;
    case OP_LOGIN_ServerStatusAlreadyLoggedIn: ret = "OP_LOGIN_ServerStatusAlreadyLoggedIn"; break;
    case OP_LOGIN_ServerStatusAccept: ret = "OP_LOGIN_ServerStatusAccept"; break;
    case OP_LOGIN_Banner: ret = "OP_LOGIN_Banner"; break;
    case OP_LOGIN_Version: ret = "OP_LOGIN_Version"; break;
    default: ret = "UNKNOWN"; break;
    }
    return ret;
}

const char* enum2str_zop(int e)
{
    const char* ret;
    switch (e)
    {
    case ZOP_None: ret = "ZOP_None"; break;
    case ZOP_UDP_OpenPort: ret = "ZOP_UDP_OpenPort"; break;
    case ZOP_UDP_TerminateThread: ret = "ZOP_UDP_TerminateThread"; break;
    case ZOP_UDP_NewClient: ret = "ZOP_UDP_NewClient"; break;
    case ZOP_UDP_DropClient: ret = "ZOP_UDP_DropClient"; break;
    case ZOP_UDP_ToServerPacket: ret = "ZOP_UDP_ToServerPacket"; break;
    case ZOP_UDP_ToClientPacketScheduled: ret = "ZOP_UDP_ToClientPacketScheduled"; break;
    case ZOP_UDP_ToClientPacketImmediate: ret = "ZOP_UDP_ToClientPacketImmediate"; break;
    case ZOP_UDP_ClientDisconnect: ret = "ZOP_UDP_ClientDisconnect"; break;
    case ZOP_UDP_ClientLinkdead: ret = "ZOP_UDP_ClientLinkdead"; break;
    case ZOP_LOG_Write: ret = "ZOP_LOG_Write"; break;
    case ZOP_LOG_OpenFile: ret = "ZOP_LOG_OpenFile"; break;
    case ZOP_LOG_CloseFile: ret = "ZOP_LOG_CloseFile"; break;
    case ZOP_LOG_TerminateThread: ret = "ZOP_LOG_TerminateThread"; break;
    case ZOP_DB_TerminateThread: ret = "ZOP_DB_TerminateThread"; break;
    case ZOP_DB_OpenDatabase: ret = "ZOP_DB_OpenDatabase"; break;
    case ZOP_DB_CloseDatabase: ret = "ZOP_DB_CloseDatabase"; break;
    case ZOP_DB_QueryLoginCredentials: ret = "ZOP_DB_QueryLoginCredentials"; break;
    case ZOP_DB_QueryLoginNewAccount: ret = "ZOP_DB_QueryLoginNewAccount"; break;
    case ZOP_LOGIN_TerminateThread: ret = "ZOP_LOGIN_TerminateThread"; break;
    case ZOP_LOGIN_NewServer: ret = "ZOP_LOGIN_NewServer"; break;
    case ZOP_LOGIN_RemoveServer: ret = "ZOP_LOGIN_RemoveServer"; break;
    case ZOP_LOGIN_UpdateServerPlayerCount: ret = "ZOP_LOGIN_UpdateServerPlayerCount"; break;
    case ZOP_LOGIN_UpdateServerStatus: ret = "ZOP_LOGIN_UpdateServerStatus"; break;
    case ZOP_CS_TerminateThread: ret = "ZOP_CS_TerminateThread"; break;
    case ZOP_CS_LoginAuth: ret = "ZOP_CS_LoginAuth"; break;
    case ZOP_CS_CheckAuthTimeouts: ret = "ZOP_CS_CheckAuthTimeouts"; break;
    case ZOP_COUNT: ret = "ZOP_COUNT"; break;
    default: ret = "UNKNOWN"; break;
    }
    return ret;
}

const char* enum2str_char_select_opcode(int e)
{
    const char* ret;
    switch (e)
    {
    case OP_CS_LoginInfo: ret = "OP_CS_LoginInfo"; break;
    case OP_CS_LoginApproved: ret = "OP_CS_LoginApproved"; break;
    case OP_CS_Enter: ret = "OP_CS_Enter"; break;
    case OP_CS_ExpansionInfo: ret = "OP_CS_ExpansionInfo"; break;
    case OP_CS_CharacterInfo: ret = "OP_CS_CharacterInfo"; break;
    case OP_CS_GuildList: ret = "OP_CS_GuildList"; break;
    case OP_CS_NameApproval: ret = "OP_CS_NameApproval"; break;
    case OP_CS_CreateCharacter: ret = "OP_CS_CreateCharacter"; break;
    case OP_CS_DeleteCharacter: ret = "OP_CS_DeleteCharacter"; break;
    case OP_CS_WearChange: ret = "OP_CS_WearChange"; break;
    case OP_CS_ZoneUnavailable: ret = "OP_CS_ZoneUnavailable"; break;
    case OP_CS_MessageOfTheDay: ret = "OP_CS_MessageOfTheDay"; break;
    case OP_CS_TimeOfDay: ret = "OP_CS_TimeOfDay"; break;
    case OP_CS_ZoneAddress: ret = "OP_CS_ZoneAddress"; break;
    case OP_CS_Echo1: ret = "OP_CS_Echo1"; break;
    case OP_CS_Echo2: ret = "OP_CS_Echo2"; break;
    case OP_CS_Echo3: ret = "OP_CS_Echo3"; break;
    case OP_CS_Echo4: ret = "OP_CS_Echo4"; break;
    case OP_CS_Echo5: ret = "OP_CS_Echo5"; break;
    default: ret = "UNKNOWN"; break;
    }
    return ret;
}

const char* enum2str_err(int e)
{
    const char* ret;
    switch (e)
    {
    case ERR_None: ret = "ERR_None"; break;
    case ERR_Generic: ret = "ERR_Generic"; break;
    case ERR_CouldNotInit: ret = "ERR_CouldNotInit"; break;
    case ERR_CouldNotOpen: ret = "ERR_CouldNotOpen"; break;
    case ERR_Invalid: ret = "ERR_Invalid"; break;
    case ERR_OutOfMemory: ret = "ERR_OutOfMemory"; break;
    case ERR_OutOfSpace: ret = "ERR_OutOfSpace"; break;
    case ERR_Timeout: ret = "ERR_Timeout"; break;
    case ERR_IO: ret = "ERR_IO"; break;
    case ERR_Again: ret = "ERR_Again"; break;
    case ERR_NotInitialized: ret = "ERR_NotInitialized"; break;
    case ERR_CouldNotCreate: ret = "ERR_CouldNotCreate"; break;
    case ERR_Semaphore: ret = "ERR_Semaphore"; break;
    case ERR_FileOperation: ret = "ERR_FileOperation"; break;
    case ERR_Fatal: ret = "ERR_Fatal"; break;
    case ERR_WouldBlock: ret = "ERR_WouldBlock"; break;
    case ERR_OutOfBounds: ret = "ERR_OutOfBounds"; break;
    case ERR_Compression: ret = "ERR_Compression"; break;
    case ERR_Library: ret = "ERR_Library"; break;
    case ERR_Lua: ret = "ERR_Lua"; break;
    case ERR_EndOfFile: ret = "ERR_EndOfFile"; break;
    case ERR_NotFound: ret = "ERR_NotFound"; break;
    case ERR_NoResource: ret = "ERR_NoResource"; break;
    case ERR_Mismatch: ret = "ERR_Mismatch"; break;
    case ERR_COUNT: ret = "ERR_COUNT"; break;
    default: ret = "UNKNOWN"; break;
    }
    return ret;
}

const char* enum2str_login_server_status(int e)
{
    const char* ret;
    switch (e)
    {
    case LOGIN_SERVER_STATUS_Up: ret = "LOGIN_SERVER_STATUS_Up"; break;
    case LOGIN_SERVER_STATUS_Down: ret = "LOGIN_SERVER_STATUS_Down"; break;
    case LOGIN_SERVER_STATUS_Locked: ret = "LOGIN_SERVER_STATUS_Locked"; break;
    default: ret = "UNKNOWN"; break;
    }
    return ret;
}
