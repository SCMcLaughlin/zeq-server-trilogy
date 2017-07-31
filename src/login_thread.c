
#include "login_thread.h"
#include "define_netcode.h"
#include "aligned.h"
#include "bit.h"
#include "buffer.h"
#include "db_thread.h"
#include "login_client.h"
#include "login_crypto.h"
#include "login_packet_struct.h"
#include "tlg_packet.h"
#include "util_alloc.h"
#include "util_atomic.h"
#include "util_random.h"
#include "util_thread.h"
#include "zpacket.h"
#include "enum_account_status.h"
#include "enum_login_opcode.h"
#include "enum_login_server_rank.h"
#include "enum_login_server_status.h"
#include "enum_zop.h"
#include "enum2str.h"

#define LOGIN_THREAD_UDP_PORT 5998
#define LOGIN_THREAD_LOG_PATH "log/login_thread.txt"
#define LOGIN_THREAD_VERSION_STRING "8-09-2001 14:25"
#define LOGIN_THREAD_BAD_CREDENTIAL_STRING "Error: Invalid username or password"
#define LOGIN_THREAD_SUSPENDED_STRING "Error: Your account has been suspended from the selected server"
#define LOGIN_THREAD_BANNED_STRING "Error: Your account has been banned from the selected server"
#define LOGIN_THREAD_DEFAULT_BANNER_MESSAGE "Welcome to ZEQ!"
#define LOGIN_THREAD_MAX_SERVER_COUNT 255
#define LOGIN_THREAD_SERVER_DOWN -1
#define LOGIN_THREAD_SERVER_LOCKED -2

typedef struct {
    int             serverId;
    int8_t          status;
    int8_t          rank;
    bool            isLocal; /* Relative to the machine running zeq-trilogy-server */
    int             playerCount;
    StaticBuffer*   name;
    StaticBuffer*   remoteIpAddr;
    StaticBuffer*   localIpAddr;
} LoginServer;

typedef struct {
    bool        isLocal;
    uint8_t     nameLength;
    uint8_t     remoteLength;
    uint8_t     localLength;
} LoginServerLengths;

struct LoginThread {
    uint32_t            clientCount;
    uint32_t            serverCount;
    int                 nextQueryId;
    atomic32_t          nextServerId;
    int                 dbId;
    int                 logId;
    LoginClient**       clients;
    LoginServer*        servers;
    LoginServerLengths* serverLengths;
    StaticBuffer*       bannerMsg;
    StaticBuffer*       loopbackAddr;
    RingBuf*            loginQueue;
    RingBuf*            udpQueue;
    RingBuf*            dbQueue;
    RingBuf*            logQueue;
    /* Cached packets that are more or less static for all clients */
    TlgPacket*          packetVersion;
    TlgPacket*          packetBanner;
    TlgPacket*          packetErrBadCredentials;
    TlgPacket*          packetErrSuspended;
    TlgPacket*          packetErrBanned;
    TlgPacket*          packetLoginAccepted;
};

static void login_thread_handle_new_server(LoginThread* login, ZPacket* zpacket)
{
    LoginServer* server;
    LoginServerLengths* lengths;
    uint32_t index = login->serverCount;
    
    if (bit_is_pow2_or_zero(index))
    {
        uint32_t cap = (index == 0) ? 1 : index * 2;
        LoginServer* servers;
        LoginServerLengths* lengths;
        
        servers = realloc_array_type(login->servers, cap, LoginServer);
        if (!servers) goto error;
        login->servers = servers;
        
        lengths = realloc_array_type(login->serverLengths, cap, LoginServerLengths);
        if (!lengths) goto error;
        login->serverLengths = lengths;
    }
    
    lengths = &login->serverLengths[index];
    lengths->isLocal = zpacket->login.zNewServer.isLocal;
    lengths->nameLength = sbuf_length(zpacket->login.zNewServer.serverName);
    lengths->remoteLength = sbuf_length(zpacket->login.zNewServer.remoteIpAddr);
    lengths->localLength = sbuf_length(zpacket->login.zNewServer.localIpAddr);
    
    server = &login->servers[index];
    login->serverCount = index + 1;
    
    server->serverId = zpacket->login.zNewServer.serverId;
    server->status = zpacket->login.zNewServer.initialStatus;
    server->rank = zpacket->login.zNewServer.rank;
    server->isLocal = zpacket->login.zNewServer.isLocal;
    server->playerCount = 0;
    server->name = zpacket->login.zNewServer.serverName;
    server->remoteIpAddr = zpacket->login.zNewServer.remoteIpAddr;
    server->localIpAddr = zpacket->login.zNewServer.localIpAddr;
    
    log_writef(login->logQueue, login->logId, "New LoginServer: \"%s\", remote ip: %s, local ip: %s, isLocal: %s",
        sbuf_str(server->name), sbuf_str(server->remoteIpAddr), sbuf_str(server->localIpAddr), (server->isLocal) ? "true" : "false");
    
    return;
    
error:
    sbuf_drop(zpacket->login.zNewServer.serverName);
    sbuf_drop(zpacket->login.zNewServer.remoteIpAddr);
    sbuf_drop(zpacket->login.zNewServer.localIpAddr);
}

static bool login_thread_is_client_valid(LoginThread* login, LoginClient* loginc)
{
    if (loginc)
    {
        LoginClient** clients = login->clients;
        uint32_t n = login->clientCount;
        uint32_t i;
        
        for (i = 0; i < n; i++)
        {
            if (clients[i] == loginc)
                return true;
        }
    }
    
    return false;
}

static LoginClient* login_thread_remove_client_object(LoginThread* login, LoginClient* loginc)
{
    if (loginc)
    {
        LoginClient** clients = login->clients;
        uint32_t n = login->clientCount;
        uint32_t i;
        
        for (i = 0; i < n; i++)
        {
            if (clients[i] == loginc)
            {
                /* Pop and swap */
                n--;
                clients[i] = clients[n];
                break;
            }
        }
        
        login->clientCount = n;
        loginc_destroy(loginc);
    }
    
    return NULL;
}

static bool login_thread_drop_client(LoginThread* login, LoginClient* loginc)
{
    ZPacket cmd;
    int rc;
    
    cmd.udp.zDropClient.ipAddress = loginc_get_ip_addr(loginc);
    cmd.udp.zDropClient.packet = NULL;
    
    rc = ringbuf_push(login->udpQueue, ZOP_UDP_DropClient, &cmd);
    
    if (rc)
    {
        uint32_t ip = cmd.udp.zDropClient.ipAddress.ip;
        
        log_writef(login->logQueue, login->logId, "login_thread_drop_client: failed to inform UdpThread to drop client from %u.%u.%u.%u:%u, keeping client alive for now",
            (ip >> 0) & 0xff, (ip >> 8) & 0xff, (ip >> 16) & 0xff, (ip >> 24) & 0xff, cmd.udp.zDropClient.ipAddress.port);
    
        return false;
    }
    
    return true;
}

static void login_thread_handle_new_client(LoginThread* login, ZPacket* zpacket)
{
    LoginClient* loginc = loginc_init(zpacket);
    uint32_t index = login->clientCount;
    
    if (bit_is_pow2_or_zero(index))
    {
        uint32_t cap = (index == 0) ? 1 : index * 2;
        LoginClient** clients = realloc_array_type(login->clients, cap, LoginClient*);

        if (!clients)
        {
            if (!login_thread_drop_client(login, loginc))
                loginc_destroy(loginc);
            return;
        }

        login->clients = clients;
    }

    login->clientCount = index + 1;
    login->clients[index] = loginc;
}

static int login_thread_handle_op_credentials(LoginThread* login, LoginClient* loginc, ZPacket* zpacket)
{
    ToServerPacket packet;
    int rc;
    
    packet.length = zpacket->udp.zToServerPacket.length;
    packet.data = zpacket->udp.zToServerPacket.data;
    
    rc = loginc_handle_op_credentials(loginc, &packet);
    
    if (rc == ERR_Again)
    {
        rc = ERR_None;
    }
    else if (rc)
    {
        loginc_schedule_packet(loginc, login->udpQueue, login->packetErrBadCredentials);
    }
    else
    {
        ZPacket zpacket;
        StaticBuffer* acctName = loginc_get_account_name(loginc);
        
        zpacket.db.zQuery.dbId = login->dbId;
        zpacket.db.zQuery.queryId = login->nextQueryId++;
        zpacket.db.zQuery.replyQueue = login->loginQueue;
        zpacket.db.zQuery.qLoginCredentials.client = loginc;
        zpacket.db.zQuery.qLoginCredentials.accountName = acctName;
        
        sbuf_grab(acctName);
        
        rc = db_queue_query(login->dbQueue, ZOP_DB_QueryLoginCredentials, &zpacket);
        
        if (rc)
        {
            sbuf_drop(acctName);
        }
    }
    
    return rc;
}

static bool login_thread_account_auto_create(LoginThread* login, LoginClient* loginc)
{
#ifdef ZEQ_LOGIN_DISABLE_ACCOUNT_AUTO_CREATE
    (void)login;
    (void)loginc;
    return false;
#else
    StaticBuffer* accountName = loginc_get_account_name(loginc);
    StaticBuffer* password = loginc_get_password(loginc);
    ZPacket zpacket;
    int rc;
    
    zpacket.db.zQuery.dbId = login->dbId;
    zpacket.db.zQuery.queryId = login->nextQueryId++;
    zpacket.db.zQuery.replyQueue = login->loginQueue;
    zpacket.db.zQuery.qLoginNewAccount.client = loginc;
    zpacket.db.zQuery.qLoginNewAccount.accountName = accountName;
    
    random_bytes(zpacket.db.zQuery.qLoginNewAccount.salt, LOGIN_CRYPTO_SALT_SIZE);
    login_crypto_hash(sbuf_str(password), sbuf_length(password), zpacket.db.zQuery.qLoginNewAccount.salt,
        LOGIN_CRYPTO_SALT_SIZE, zpacket.db.zQuery.qLoginNewAccount.passwordHash);
    
    loginc_clear_password(loginc);
    sbuf_grab(accountName);
    
    rc = db_queue_query(login->dbQueue, ZOP_DB_QueryLoginNewAccount, &zpacket);
    
    if (rc)
    {
        sbuf_drop(accountName);
        return false;
    }
    
    return true;
#endif
}

static int login_thread_check_db_error(LoginThread* login, LoginClient* loginc, ZPacket* zpacket, const char* funcName)
{
    if (zpacket->db.zResult.hadErrorUnprocessed)
    {
        log_writef(login->logQueue, login->logId, "WARNING: %s: database reported error, query unprocessed", funcName);
        return ERR_Invalid;
    }
    
    if (!login_thread_is_client_valid(login, loginc))
    {
        log_writef(login->logQueue, login->logId, "WARNING: %s: received result for LoginClient that no longer exists", funcName);
        return ERR_Invalid;
    }
    
    if (zpacket->db.zResult.hadError)
    {
        log_writef(login->logQueue, login->logId, "WARNING: %s: database reported error", funcName);
        return ERR_Generic;
    }
    
    return ERR_None;
}

static void login_thread_handle_db_login_credentials(LoginThread* login, ZPacket* zpacket)
{
    LoginClient* loginc = (LoginClient*)zpacket->db.zResult.rLoginCredentials.client;
    int rc;
    
    switch (login_thread_check_db_error(login, loginc, zpacket, FUNC_NAME))
    {
    case ERR_Invalid:
        return;
    
    case ERR_Generic:
        goto drop_client;
    
    case ERR_None:
        break;
    }
    
    if (zpacket->db.zResult.rLoginCredentials.acctId < 0)
    {
        /* Try to auto-create an account with these credentials */
        if (login_thread_account_auto_create(login, loginc))
            return;
        
        loginc_schedule_packet(loginc, login->udpQueue, login->packetErrBadCredentials);
        goto drop_client;
    }
    
    rc = loginc_handle_db_credentials(loginc, zpacket, login->udpQueue, login->packetErrBadCredentials);
    if (rc) 
    {
        if (rc != ERR_Mismatch)
            log_writef(login->logQueue, login->logId, "login_thread_handle_db_login_credentials: successful client login could not be processed: %s", enum2str_err(rc));
        goto drop_client;
    }
    
    return;
    
drop_client:
    login_thread_drop_client(login, loginc);
}

static void login_thread_handle_db_new_account(LoginThread* login, ZPacket* zpacket)
{
    LoginClient* loginc = (LoginClient*)zpacket->db.zResult.rLoginNewAccount.client;
    int rc;
    
    switch (login_thread_check_db_error(login, loginc, zpacket, FUNC_NAME))
    {
    case ERR_Invalid:
        return;
    
    case ERR_Generic:
        goto drop_client;
    
    case ERR_None:
        break;
    }
    
    rc = loginc_send_session_packet(loginc, login->udpQueue, zpacket->db.zResult.rLoginNewAccount.acctId);
    if (rc)
    {
        log_writef(login->logQueue, login->logId, "login_thread_handle_db_new_account: successfully created new account, but session packet could not be sent: %s", enum2str_err(rc));
        goto drop_client;
    }
    
    return;
    
drop_client:
    login_thread_drop_client(login, loginc);
}

static uint32_t login_thread_calc_server_list_length(LoginThread* login, bool isLocal)
{
    LoginServerLengths* serverLengths = login->serverLengths;
    uint32_t count = login->serverCount;
    uint32_t length;
    uint32_t i;
    
    length = sizeof(PSLogin_ServerListHeader) + sizeof(PSLogin_ServerListFooter);
    
    for (i = 0; i < count; i++)
    {
        LoginServerLengths* lengths = &serverLengths[i];
        
        length += (isLocal && lengths->isLocal) ? lengths->localLength : lengths->remoteLength;
        length += lengths->nameLength;
        length += sizeof(PSLogin_ServerFooter);
    }
    
    return length;
}

static void login_thread_write_server_list(LoginThread* login, bool isLocal, TlgPacket* packet)
{
    LoginServer* servers = login->servers;
    uint32_t count = login->serverCount;
    Aligned a;
    uint32_t i;
    
    aligned_init(&a, packet_data(packet), packet_length(packet));
    
    /* PSLogin_ServerListHeader */
    /* serverCount */
    aligned_write_uint8(&a, count);
    /* unknown (2 bytes) */
    aligned_write_zeroes(&a, sizeof(byte) * 2);
    /* showNumPlayers */
    aligned_write_uint8(&a, 0xff); /* Show actual numbers rather than just "UP" */
    
    /* Server entries */
    for (i = 0; i < count; i++)
    {
        LoginServer* server = &servers[i];
        int playerCount;
        
        switch (server->status)
        {
        case LOGIN_SERVER_STATUS_Down:
            playerCount = LOGIN_THREAD_SERVER_DOWN;
            break;
        
        case LOGIN_SERVER_STATUS_Locked:
            playerCount = LOGIN_THREAD_SERVER_LOCKED;
            break;
        
        default:
            playerCount = server->playerCount;
            break;
        }
        
        /* Variable-length strings */
        /* server name */
        aligned_write_sbuf_null_terminated(&a, server->name);
        /* ip address */
        aligned_write_sbuf_null_terminated(&a, (isLocal && server->isLocal) ? server->localIpAddr : server->remoteIpAddr);
    
        /* PSLogin_ServerFooter */
        /* isGreenName */
        aligned_write_bool(&a, (server->rank != LOGIN_SERVER_RANK_Standard));
        /* playerCount */
        aligned_write_int32(&a, playerCount);
    }
    
    /* PSLogin_ServerListFooter */
    /* admin */
    aligned_write_uint8(&a, 0);
    /* zeroesA (8 bytes) */
    aligned_write_zeroes(&a, sizeof(byte) * 8);
    /* kunark */
    aligned_write_uint8(&a, 1);
    /* velious */
    aligned_write_uint8(&a, 1);
    /* zeroesB (12 bytes) */
    aligned_write_zeroes(&a, sizeof(byte) * 12);
}

static int login_thread_handle_op_server_list(LoginThread* login, LoginClient* loginc)
{
    TlgPacket* packet;
    uint32_t length;
    bool isLocal;
    
    if (!loginc_is_authorized(loginc))
        return ERR_Invalid;
    
    isLocal = loginc_is_local(loginc);
    length = login_thread_calc_server_list_length(login, isLocal);
    packet = packet_create(OP_LOGIN_ServerList, length);
    if (!packet) return ERR_OutOfMemory;
    
    login_thread_write_server_list(login, isLocal, packet);
    
    return loginc_schedule_packet(loginc, login->udpQueue, packet);
}

static int login_thread_handle_op_server_status_request(LoginThread* login, LoginClient* loginc, ZPacket* zpacket)
{
    const char* ipAddress = (const char*)zpacket->udp.zToServerPacket.data;
    RingBuf* udpQueue;
    uint32_t length = zpacket->udp.zToServerPacket.length;
    int status;
    int rc;
    
    if (!ipAddress || length == 0 || !loginc_is_authorized(loginc))
        return ERR_Invalid;
    
    /*fixme:
        To support multiple servers, we would use the ipAddress from the client to look up 
        which server they want to connect to, then have some back and forth with that server
        to see if they accept this client logging on.
    
        Since this is just a standalone one-to-one login server for now, we just respond
        immediately with what we already know about the client.
    */
    
    status = loginc_account_status(loginc);
    udpQueue = login->udpQueue;
    
    switch (status)
    {
    case ACCT_STATUS_Banned:
        rc = loginc_schedule_packet(loginc, udpQueue, login->packetErrBanned);
        break;
    
    case ACCT_STATUS_Suspended:
        rc = loginc_schedule_packet(loginc, udpQueue, login->packetErrSuspended);
        break;
    
    default:
        rc = loginc_schedule_packet(loginc, udpQueue, login->packetLoginAccepted);
        break;
    }
    
    return rc;
}

static int login_thread_handle_op_session_key(LoginThread* login, LoginClient* loginc)
{
    const char* sessionKey;
    TlgPacket* packet;
    Aligned a;
    
    if (!loginc_is_authorized(loginc))
        return ERR_Invalid;
    
    packet = packet_create_type(OP_LOGIN_SessionKey, PSLogin_SessionKey);
    if (!packet) return ERR_OutOfMemory;
    
    loginc_generate_session_key(loginc);
    sessionKey = loginc_get_session_key(loginc);
    
    aligned_init(&a, packet_data(packet), packet_length(packet));
    
    /* PSLogin_SessionKey */
    /* unknown */
    aligned_write_uint8(&a, 0);
    /* sessionKey */
    aligned_write_buffer(&a, sessionKey, sizeof_field(PSLogin_SessionKey, sessionKey));
    
    /*fixme: inform the MainThread about this login with this session key */
    
    return loginc_schedule_packet(loginc, login->udpQueue, packet);
}

static void login_thread_handle_packet(LoginThread* login, ZPacket* zpacket)
{
    LoginClient* loginc = (LoginClient*)zpacket->udp.zToServerPacket.clientObject;
    byte* data = zpacket->udp.zToServerPacket.data;
    int rc = ERR_None;
    
    switch (zpacket->udp.zToServerPacket.opcode)
    {
    case OP_LOGIN_Version:
        rc = loginc_handle_op_version(loginc, login->udpQueue, login->packetVersion);
        break;

    case OP_LOGIN_Credentials:
        rc = login_thread_handle_op_credentials(login, loginc, zpacket);
        break;

    case OP_LOGIN_Banner:
        rc = loginc_handle_op_banner(loginc, login->udpQueue, login->packetBanner);
        break;

    case OP_LOGIN_ServerList:
        rc = login_thread_handle_op_server_list(login, loginc);
        break;

    case OP_LOGIN_ServerStatusRequest:
        rc = login_thread_handle_op_server_status_request(login, loginc, zpacket);
        break;

    case OP_LOGIN_SessionKey:
        rc = login_thread_handle_op_session_key(login, loginc);
        break;
    
    case OP_LOGIN_Exit:
        login_thread_drop_client(login, loginc);
        break;

    default:
        log_writef(login->logQueue, login->logId, "WARNING: login_thread_handle_packet: received unexpected login opcode: %04x", zpacket->udp.zToServerPacket.opcode);
        break;
    }
    
    if (rc)
    {
        uint32_t ip = loginc_get_ip(loginc);
        uint16_t port = loginc_get_port(loginc);
        
        log_writef(login->logQueue, login->logId, "login_thread_handle_packet: got error %s while processing packet with opcode %s from client from %u.%u.%u.%u:%u, disconnecting them to maintain consistency",
            enum2str_err(rc), enum2str_login_opcode(zpacket->udp.zToServerPacket.opcode), (ip >> 0) & 0xff, (ip >> 8) & 0xff, (ip >> 16) & 0xff, (ip >> 24) & 0xff, port);
        
        login_thread_drop_client(login, loginc);
    }

    if (data) free(data);
}

static void login_thread_proc(void* ptr)
{
    LoginThread* login = (LoginThread*)ptr;
    RingBuf* loginQueue = login->loginQueue;
    ZPacket zpacket;
    int zop;
    int rc;
    
    for (;;)
    {
        /* The login thread is purely packet-driven, always blocks until something needs to be done */
        rc = ringbuf_wait_for_packet(loginQueue, &zop, &zpacket);
        
        if (rc)
        {
            log_writef(login->logQueue, login->logId, "login_thread_proc: ringbuf_wait_for_packet() returned error: %s", enum2str_err(rc));
            break;
        }
        
        switch (zop)
        {
        case ZOP_UDP_ToServerPacket:
            login_thread_handle_packet(login, &zpacket);
            break;

        case ZOP_UDP_NewClient:
            login_thread_handle_new_client(login, &zpacket);
            break;
        
        case ZOP_UDP_ClientLinkdead:
            break;
        
        case ZOP_UDP_ClientDisconnect:
            login_thread_remove_client_object(login, (LoginClient*)zpacket.udp.zClientDisconnect.clientObject);
            break;
        
        case ZOP_DB_QueryLoginCredentials:
            login_thread_handle_db_login_credentials(login, &zpacket);
            break;
        
        case ZOP_DB_QueryLoginNewAccount:
            login_thread_handle_db_new_account(login, &zpacket);
            break;
        
        case ZOP_LOGIN_TerminateThread:
            return;
        
        case ZOP_LOGIN_NewServer:
            login_thread_handle_new_server(login, &zpacket);
            break;
        
        case ZOP_LOGIN_RemoveServer:
            break;
        
        case ZOP_LOGIN_UpdateServerPlayerCount:
            break;
        
        case ZOP_LOGIN_UpdateServerStatus:
            break;
        
        default:
            log_writef(login->logQueue, login->logId, "WARNING: login_thread_proc: received unexpected zop: %s", enum2str_zop(zop));
            break;
        }
    }
}

static TlgPacket* login_create_cached_packet_string(uint16_t opcode, const char* str, uint32_t len)
{
    TlgPacket* packet = packet_create(opcode, len);

    if (!packet) goto ret;

    memcpy(packet_data(packet), str, len);
    packet_grab(packet);

ret:
    return packet;
}

#define login_create_cached_packet_string_literal(op, str) login_create_cached_packet_string((op), str, sizeof(str))

static int login_create_cached_packets(LoginThread* login)
{
    Aligned a;

    login->packetVersion = login_create_cached_packet_string_literal(OP_LOGIN_Version, LOGIN_THREAD_VERSION_STRING);
    if (!login->packetVersion) goto fail;

    login->packetErrBadCredentials = login_create_cached_packet_string_literal(OP_LOGIN_Error, LOGIN_THREAD_BAD_CREDENTIAL_STRING);
    if (!login->packetErrBadCredentials) goto fail;

    login->packetErrSuspended = login_create_cached_packet_string_literal(OP_LOGIN_Error, LOGIN_THREAD_SUSPENDED_STRING);
    if (!login->packetErrSuspended) goto fail;

    login->packetErrBanned = login_create_cached_packet_string_literal(OP_LOGIN_Error, LOGIN_THREAD_BANNED_STRING);
    if (!login->packetErrBanned) goto fail;

    /* Banner message has an extra field */
    login->packetBanner = packet_create(OP_LOGIN_Banner, sizeof(uint32_t) + sizeof(LOGIN_THREAD_DEFAULT_BANNER_MESSAGE));
    if (!login->packetBanner) goto fail;
    packet_grab(login->packetBanner);

    aligned_init(&a, packet_data(login->packetBanner), sizeof(uint32_t) + sizeof(LOGIN_THREAD_DEFAULT_BANNER_MESSAGE));

    /* unknown */
    aligned_write_uint32(&a, 1);
    /* message */
    aligned_write_literal_null_terminated(&a, LOGIN_THREAD_DEFAULT_BANNER_MESSAGE);
    
    /* Login accepted packet has no content, just the opcode */
    login->packetLoginAccepted = packet_create_opcode_only(OP_LOGIN_ServerStatusAccept);
    if (!login->packetLoginAccepted) goto fail;
    packet_grab(login->packetLoginAccepted);

    return ERR_None;
fail:
    return ERR_OutOfMemory;
}

LoginThread* login_create(LogThread* log, RingBuf* dbQueue, int dbId, RingBuf* udpQueue)
{
    LoginThread* login = alloc_type(LoginThread);
    int rc;

    if (!login) goto fail_alloc;

    login->clientCount = 0;
    login->serverCount = 0;
    login->nextQueryId = 1;
    atomic32_set(&login->nextServerId, 1);
    login->dbId = dbId;
    login->clients = NULL;
    login->servers = NULL;
    login->serverLengths = NULL;
    login->bannerMsg = NULL;
    login->loopbackAddr = NULL;
    login->loginQueue = NULL;
    login->udpQueue = udpQueue;
    login->dbQueue = dbQueue;
    login->logQueue = log_get_queue(log);

    login->packetVersion = NULL;
    login->packetBanner = NULL;
    login->packetErrBadCredentials = NULL;
    login->packetErrSuspended = NULL;
    login->packetErrBanned = NULL;
    login->packetLoginAccepted = NULL;

    rc = log_open_file_literal(log, &login->logId, LOGIN_THREAD_LOG_PATH);
    if (rc) goto fail;

    login->loginQueue = ringbuf_create_type(ZPacket, 1024);
    if (!login->loginQueue) goto fail;
    
    login->loopbackAddr = sbuf_create_from_literal("127.0.0.1");
    if (!login->loopbackAddr) goto fail;
    sbuf_grab(login->loopbackAddr);

    rc = login_create_cached_packets(login);
    if (rc) goto fail;

    rc = thread_start(login_thread_proc, login);
    if (rc) goto fail;

    return login;

fail:
    login_destroy(login);
fail_alloc:
    return NULL;
}

static void login_free_clients(LoginThread* login)
{
    LoginClient** clients = login->clients;
        
    if (clients)
    {
        uint32_t n = login->clientCount;
        uint32_t i;
        
        for (i = 0; i < n; i++)
        {
            clients[i] = loginc_destroy(clients[i]);
        }
        
        free(clients);
        login->clients = NULL;
    }
}

static void login_free_servers(LoginThread* login)
{
    LoginServer* servers = login->servers;
    
    if (servers)
    {
        uint32_t count = login->serverCount;
        uint32_t i;
        
        for (i = 0; i < count; i++)
        {
            LoginServer* server = &servers[i];
            
            server->name = sbuf_drop(server->name);
            server->remoteIpAddr = sbuf_drop(server->remoteIpAddr);
            server->localIpAddr = sbuf_drop(server->localIpAddr);
        }
        
        free(servers);
        login->servers = NULL;
    }
}

LoginThread* login_destroy(LoginThread* login)
{
    if (login)
    {
        login_free_clients(login);
        login_free_servers(login);
        free_if_exists(login->serverLengths);
        
        login->bannerMsg = sbuf_drop(login->bannerMsg);
        login->loopbackAddr = sbuf_drop(login->loopbackAddr);
        
        login->loginQueue = ringbuf_destroy(login->loginQueue);

        login->packetVersion = packet_drop(login->packetVersion);
        login->packetBanner = packet_drop(login->packetBanner);
        login->packetErrBadCredentials = packet_drop(login->packetErrBadCredentials);
        login->packetErrSuspended = packet_drop(login->packetErrSuspended);
        login->packetErrBanned = packet_drop(login->packetErrBanned);
        login->packetLoginAccepted = packet_drop(login->packetLoginAccepted);

        log_close_file(login->logQueue, login->logId);
        free(login);
    }

    return NULL;
}

RingBuf* login_get_queue(LoginThread* login)
{
    return login->loginQueue;
}

int login_add_server(LoginThread* login, int* outServerId, const char* name, const char* remoteIp, const char* localIp, int8_t rank, int8_t status, bool isLocal)
{
    ZPacket zpacket;
    int serverId;
    int rc;
    
    if (!name) return ERR_Invalid;
    
    serverId = atomic32_add(&login->nextServerId, 1);
    
    if (outServerId)
        *outServerId = serverId;
    
    zpacket.login.zNewServer.initialStatus = status;
    zpacket.login.zNewServer.rank = rank;
    zpacket.login.zNewServer.isLocal = isLocal;
    zpacket.login.zNewServer.serverId = serverId;
    
    zpacket.login.zNewServer.serverName = sbuf_create(name, strlen(name));
    zpacket.login.zNewServer.remoteIpAddr = (remoteIp) ? sbuf_create(remoteIp, strlen(remoteIp)) : login->loopbackAddr;
    zpacket.login.zNewServer.localIpAddr = (localIp) ? sbuf_create(localIp, strlen(localIp)) : login->loopbackAddr;
    
    sbuf_grab(zpacket.login.zNewServer.serverName);
    sbuf_grab(zpacket.login.zNewServer.remoteIpAddr);
    sbuf_grab(zpacket.login.zNewServer.localIpAddr);
    
    if (!zpacket.login.zNewServer.serverName || !zpacket.login.zNewServer.remoteIpAddr || !zpacket.login.zNewServer.localIpAddr)
    {
        rc = ERR_OutOfMemory;
        goto error;
    }
    
    if (sbuf_length(zpacket.login.zNewServer.serverName) > UINT8_MAX || sbuf_length(zpacket.login.zNewServer.remoteIpAddr) > UINT8_MAX || sbuf_length(zpacket.login.zNewServer.localIpAddr) > UINT8_MAX)
    {
        rc = ERR_OutOfBounds;
        goto error;
    }
    
    rc = ringbuf_push(login->loginQueue, ZOP_LOGIN_NewServer, &zpacket);
    if (rc) goto error;
    
    return rc;
    
error:
    sbuf_drop(zpacket.login.zNewServer.serverName);
    sbuf_drop(zpacket.login.zNewServer.remoteIpAddr);
    sbuf_drop(zpacket.login.zNewServer.localIpAddr);
    return rc;
}
