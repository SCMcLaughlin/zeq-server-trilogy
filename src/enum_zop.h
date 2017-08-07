
#ifndef ENUM_ZOP_H
#define ENUM_ZOP_H

enum ZOP
{
    ZOP_None,
    /* UDP thread */
    ZOP_UDP_OpenPort,
    ZOP_UDP_TerminateThread,
    ZOP_UDP_NewClient,
    ZOP_UDP_DropClient,
    ZOP_UDP_ToServerPacket,
    ZOP_UDP_ToClientPacketScheduled,
    ZOP_UDP_ToClientPacketImmediate,
    ZOP_UDP_ClientDisconnect,
    ZOP_UDP_ClientLinkdead,
    ZOP_UDP_ReplaceClientObject,
    /* Log thread */
    ZOP_LOG_Write,
    ZOP_LOG_OpenFile,
    ZOP_LOG_CloseFile,
    ZOP_LOG_TerminateThread,
    /* DB thread */
    ZOP_DB_TerminateThread,
    ZOP_DB_OpenDatabase,
    ZOP_DB_CloseDatabase,
    ZOP_DB_QueryMainGuildList,
    ZOP_DB_QueryLoginCredentials,
    ZOP_DB_QueryLoginNewAccount,
    ZOP_DB_QueryCSCharacterInfo,
    ZOP_DB_QueryCSCharacterNameAvailable,
    ZOP_DB_QueryCSCharacterCreate,
    ZOP_DB_QueryCSCharacterDelete,
    ZOP_DB_QueryMainLoadCharacter,
    /* Login thread */
    ZOP_LOGIN_TerminateThread,
    ZOP_LOGIN_NewServer,
    ZOP_LOGIN_RemoveServer,
    ZOP_LOGIN_UpdateServerPlayerCount,
    ZOP_LOGIN_UpdateServerStatus,
    /* CharSelect thread */
    ZOP_CS_TerminateThread,
    ZOP_CS_AddGuild,
    ZOP_CS_LoginAuth,
    ZOP_CS_CheckAuthTimeouts,
    ZOP_CS_ZoneSuccess,
    ZOP_CS_ZoneFailure,
    /* Main thread */
    ZOP_MAIN_TerminateAll,
    ZOP_MAIN_ZoneFromCharSelect,
    /* Zone thread */
    ZOP_ZONE_TerminateThread,
    ZOP_ZONE_CreateZone,
    ZOP_ZONE_AddClientExpected,
    ZOP_COUNT
};

#endif/*ENUM_ZOP_H*/
