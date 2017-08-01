
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
    ZOP_COUNT
};

#endif/*ENUM_ZOP_H*/
