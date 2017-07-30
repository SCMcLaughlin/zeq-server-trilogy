
#include "login_thread.h"
#include "define_netcode.h"
#include "aligned.h"
#include "bit.h"
#include "buffer.h"
#include "db_thread.h"
#include "ringbuf.h"
#include "login_client.h"
#include "login_crypto.h"
#include "login_packet_struct.h"
#include "tlg_packet.h"
#include "util_alloc.h"
#include "util_random.h"
#include "util_thread.h"
#include "zpacket.h"
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
    uint32_t        nameLength;
    uint32_t        remoteLength;
    uint32_t        localLength;
    StaticBuffer*   name;
    StaticBuffer*   remoteIpAddr;
    StaticBuffer*   localIpAddr;
} LoginServer;

struct LoginThread {
    uint32_t        clientCount;
    uint32_t        serverCount;
    int             nextQueryId;
    int             dbId;
    int             logId;
    LoginClient**   clients;
    LoginServer*    servers;
    StaticBuffer*   bannerMsg;
    RingBuf*        loginQueue;
    RingBuf*        toClientQueue;
    RingBuf*        dbQueue;
    RingBuf*        logQueue;
    /* Cached packets that are more or less static for all clients */
    TlgPacket*      packetVersion;
    TlgPacket*      packetBanner;
    TlgPacket*      packetErrBadCredentials;
    TlgPacket*      packetErrSuspended;
    TlgPacket*      packetErrBanned;
};

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

static LoginClient* login_thread_drop_client(LoginThread* login, LoginClient* loginc)
{
    ZPacket cmd;
    int rc;
    
    cmd.udp.zDropClient.ipAddress = loginc_get_ip_addr(loginc);
    cmd.udp.zDropClient.packet = NULL;
    
    rc = ringbuf_push(login->toClientQueue, ZOP_UDP_DropClient, &cmd);
    
    if (rc)
    {
        uint32_t ip = cmd.udp.zDropClient.ipAddress.ip;
        
        log_writef(login->logQueue, login->logId, "login_thread_drop_client: failed to inform UdpThread to drop client from %u.%u.%u.%u:%u, keeping client alive for now",
            (ip >> 0) & 0xff, (ip >> 8) & 0xff, (ip >> 16) & 0xff, (ip >> 24) & 0xff, cmd.udp.zDropClient.ipAddress.port);
    }
    else
    {
        loginc = login_thread_remove_client_object(login, loginc);
    }
    
    return loginc;
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
            loginc = login_thread_drop_client(login, loginc);
            loginc_destroy(loginc); /* Unconditional destruction, in case login_thread_drop_client() fails */
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
        loginc_schedule_packet(loginc, login->toClientQueue, login->packetErrBadCredentials);
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
    
    rc = db_queue_query(login->dbQueue, login->dbId, &zpacket);
    
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
        
        loginc_schedule_packet(loginc, login->toClientQueue, login->packetErrBadCredentials);
        goto drop_client;
    }
    
    rc = loginc_handle_db_credentials(loginc, zpacket, login->toClientQueue, login->packetErrBadCredentials);
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
    
    rc = loginc_send_session_packet(loginc, login->toClientQueue, zpacket->db.zResult.rLoginNewAccount.acctId);
    if (rc)
    {
        log_writef(login->logQueue, login->logId, "login_thread_handle_db_new_account: successfully created new account, but session packet could not be sent: %s", enum2str_err(rc));
        goto drop_client;
    }
    
    return;
    
drop_client:
    login_thread_drop_client(login, loginc);
}

uint32_t login_thread_calc_server_list_length(LoginThread* login, bool isLocal)
{
    LoginServer* servers = login->servers;
    uint32_t count = login->serverCount;
    uint32_t length;
    uint32_t i;
    
    length = sizeof(PSLogin_ServerListHeader) + sizeof(PSLogin_ServerListFooter);
    
    for (i = 0; i < count; i++)
    {
        LoginServer* server = &servers[i];
        
        length += (isLocal && server->isLocal) ? server->localLength : server->remoteLength;
        length += server->nameLength;
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

int login_thread_handle_op_server_list(LoginThread* login, LoginClient* loginc)
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
    
    return loginc_schedule_packet(loginc, login->toClientQueue, packet);
}

static void login_thread_handle_packet(LoginThread* login, ZPacket* zpacket)
{
    LoginClient* loginc = (LoginClient*)zpacket->udp.zToServerPacket.clientObject;
    byte* data = zpacket->udp.zToServerPacket.data;
    int rc = ERR_None;

    switch (zpacket->udp.zToServerPacket.opcode)
    {
    case OP_LOGIN_Version:
        rc = loginc_handle_op_version(loginc, login->toClientQueue, login->packetVersion);
        break;

    case OP_LOGIN_Credentials:
        rc = login_thread_handle_op_credentials(login, loginc, zpacket);
        break;

    case OP_LOGIN_Banner:
        rc = loginc_handle_op_banner(loginc, login->toClientQueue, login->packetBanner);
        break;

    case OP_LOGIN_ServerList:
        rc = login_thread_handle_op_server_list(login, loginc);
        break;

    case OP_LOGIN_ServerStatusRequest:
        break;

    case OP_LOGIN_SessionKey:
        break;

    default:
        log_writef(login->logQueue, login->logId, "WARNING: login_thread_handle_packet: received unexpected login opcode: %04x", zpacket->udp.zToServerPacket.opcode);
        break;
    }
    
    if (rc)
    {
        uint32_t ip = loginc_get_ip(loginc);
        uint16_t port = loginc_get_port(loginc);
        
        log_writef(login->logQueue, login->logId, "login_thread_handle_packet: got errcode %i while processing packet with opcode %u from client from %u.%u.%u.%u:%u, disconnecting them to maintain consistency",
            rc, zpacket->udp.zToServerPacket.opcode, (ip >> 0) & 0xff, (ip >> 8) & 0xff, (ip >> 16) & 0xff, (ip >> 24) & 0xff, port);
        
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
            log_writef(login->logQueue, login->logId, "login_thread_proc: ringbuf_wait_for_packet() returned errcode: %i", rc);
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
        
        case ZOP_DB_QueryLoginCredentials:
            login_thread_handle_db_login_credentials(login, &zpacket);
            break;
        
        case ZOP_DB_QueryLoginNewAccount:
            login_thread_handle_db_new_account(login, &zpacket);
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

    return ERR_None;
fail:
    return ERR_OutOfMemory;
}

LoginThread* login_create(LogThread* log, RingBuf* dbQueue, int dbId)
{
    LoginThread* login = alloc_type(LoginThread);
    int rc;

    if (!login) goto fail_alloc;

    login->clientCount = 0;
    login->nextQueryId = 1;
    login->dbId = dbId;
    login->clients = NULL;
    login->bannerMsg = NULL;
    login->loginQueue = NULL;
    login->toClientQueue = NULL;
    login->dbQueue = dbQueue;
    login->logQueue = log_get_queue(log);

    login->packetVersion = NULL;
    login->packetBanner = NULL;
    login->packetErrBadCredentials = NULL;
    login->packetErrSuspended = NULL;
    login->packetErrBanned = NULL;

    rc = log_open_file_literal(log, &login->logId, LOGIN_THREAD_LOG_PATH);
    if (rc) goto fail;

    login->loginQueue = ringbuf_create_type(ZPacket, 1024);
    if (!login->loginQueue) goto fail;

    login->toClientQueue = ringbuf_create_type(ZPacket, 1024);
    if (!login->toClientQueue) goto fail;

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

LoginThread* login_destroy(LoginThread* login)
{
    if (login)
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
        
        login->bannerMsg = sbuf_drop(login->bannerMsg);

        login->packetVersion = packet_drop(login->packetVersion);
        login->packetBanner = packet_drop(login->packetBanner);
        login->packetErrBadCredentials = packet_drop(login->packetErrBadCredentials);
        login->packetErrSuspended = packet_drop(login->packetErrSuspended);
        login->packetErrBanned = packet_drop(login->packetErrBanned);

        log_close_file(login->logQueue, login->logId);
        free(login);
    }

    return NULL;
}
