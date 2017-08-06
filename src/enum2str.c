
#include "enum2str.h"
#include "enum_account_status.h"
#include "enum_char_select_opcode.h"
#include "enum_err.h"
#include "enum_login_opcode.h"
#include "enum_login_server_rank.h"
#include "enum_login_server_status.h"
#include "enum_opcode.h"
#include "enum_zop.h"

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
    case OP_CS_Ignore1: ret = "OP_CS_Ignore1"; break;
    case OP_CS_Ignore2: ret = "OP_CS_Ignore2"; break;
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

const char* enum2str_opcode(int e)
{
    const char* ret;
    switch (e)
    {
    case OP_SetDataRate: ret = "OP_SetDataRate"; break;
    case OP_ZoneEntry: ret = "OP_ZoneEntry"; break;
    case OP_PlayerProfile: ret = "OP_PlayerProfile"; break;
    case OP_Weather: ret = "OP_Weather"; break;
    case OP_InventoryRequest: ret = "OP_InventoryRequest"; break;
    case OP_ZoneInfoRequest: ret = "OP_ZoneInfoRequest"; break;
    case OP_ZoneInfo: ret = "OP_ZoneInfo"; break;
    case OP_ZoneInUnknown: ret = "OP_ZoneInUnknown"; break;
    case OP_EnterZone: ret = "OP_EnterZone"; break;
    case OP_EnteredZoneUnknown: ret = "OP_EnteredZoneUnknown"; break;
    case OP_Inventory: ret = "OP_Inventory"; break;
    case OP_PositionUpdate: ret = "OP_PositionUpdate"; break;
    case OP_ClientPositionUpdate: ret = "OP_ClientPositionUpdate"; break;
    case OP_SpawnAppearance: ret = "OP_SpawnAppearance"; break;
    case OP_Spawn: ret = "OP_Spawn"; break;
    case OP_CustomMessage: ret = "OP_CustomMessage"; break;
    case OP_SwapItem: ret = "OP_SwapItem"; break;
    case OP_MoveCoin: ret = "OP_MoveCoin"; break;
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
    case ZOP_UDP_ReplaceClientObject: ret = "ZOP_UDP_ReplaceClientObject"; break;
    case ZOP_LOG_Write: ret = "ZOP_LOG_Write"; break;
    case ZOP_LOG_OpenFile: ret = "ZOP_LOG_OpenFile"; break;
    case ZOP_LOG_CloseFile: ret = "ZOP_LOG_CloseFile"; break;
    case ZOP_LOG_TerminateThread: ret = "ZOP_LOG_TerminateThread"; break;
    case ZOP_DB_TerminateThread: ret = "ZOP_DB_TerminateThread"; break;
    case ZOP_DB_OpenDatabase: ret = "ZOP_DB_OpenDatabase"; break;
    case ZOP_DB_CloseDatabase: ret = "ZOP_DB_CloseDatabase"; break;
    case ZOP_DB_QueryMainGuildList: ret = "ZOP_DB_QueryMainGuildList"; break;
    case ZOP_DB_QueryLoginCredentials: ret = "ZOP_DB_QueryLoginCredentials"; break;
    case ZOP_DB_QueryLoginNewAccount: ret = "ZOP_DB_QueryLoginNewAccount"; break;
    case ZOP_DB_QueryCSCharacterInfo: ret = "ZOP_DB_QueryCSCharacterInfo"; break;
    case ZOP_DB_QueryCSCharacterNameAvailable: ret = "ZOP_DB_QueryCSCharacterNameAvailable"; break;
    case ZOP_DB_QueryCSCharacterCreate: ret = "ZOP_DB_QueryCSCharacterCreate"; break;
    case ZOP_DB_QueryCSCharacterDelete: ret = "ZOP_DB_QueryCSCharacterDelete"; break;
    case ZOP_DB_QueryMainLoadCharacter: ret = "ZOP_DB_QueryMainLoadCharacter"; break;
    case ZOP_LOGIN_TerminateThread: ret = "ZOP_LOGIN_TerminateThread"; break;
    case ZOP_LOGIN_NewServer: ret = "ZOP_LOGIN_NewServer"; break;
    case ZOP_LOGIN_RemoveServer: ret = "ZOP_LOGIN_RemoveServer"; break;
    case ZOP_LOGIN_UpdateServerPlayerCount: ret = "ZOP_LOGIN_UpdateServerPlayerCount"; break;
    case ZOP_LOGIN_UpdateServerStatus: ret = "ZOP_LOGIN_UpdateServerStatus"; break;
    case ZOP_CS_TerminateThread: ret = "ZOP_CS_TerminateThread"; break;
    case ZOP_CS_AddGuild: ret = "ZOP_CS_AddGuild"; break;
    case ZOP_CS_LoginAuth: ret = "ZOP_CS_LoginAuth"; break;
    case ZOP_CS_CheckAuthTimeouts: ret = "ZOP_CS_CheckAuthTimeouts"; break;
    case ZOP_CS_ZoneSuccess: ret = "ZOP_CS_ZoneSuccess"; break;
    case ZOP_CS_ZoneFailure: ret = "ZOP_CS_ZoneFailure"; break;
    case ZOP_MAIN_ZoneFromCharSelect: ret = "ZOP_MAIN_ZoneFromCharSelect"; break;
    case ZOP_ZONE_TerminateThread: ret = "ZOP_ZONE_TerminateThread"; break;
    case ZOP_ZONE_CreateZone: ret = "ZOP_ZONE_CreateZone"; break;
    case ZOP_ZONE_AddClientExpected: ret = "ZOP_ZONE_AddClientExpected"; break;
    case ZOP_COUNT: ret = "ZOP_COUNT"; break;
    default: ret = "UNKNOWN"; break;
    }
    return ret;
}
