
#include "char_select_thread.h"
#include "aligned.h"
#include "bit.h"
#include "char_select_client.h"
#include "tlg_packet.h"
#include "util_alloc.h"
#include "util_clock.h"
#include "util_thread.h"
#include "zpacket.h"
#include "char_select_packet_struct.h"
#include "enum_char_select_opcode.h"
#include "enum_zop.h"
#include "enum2str.h"

#define CHAR_SELECT_THREAD_UDP_PORT 9000
#define CHAR_SELECT_THREAD_LOG_PATH "log/char_select_thread.txt"
#define CHAR_SELECT_THREAD_AUTH_TIMEOUT_MS 10000

typedef struct {
    int64_t     accountId;
    char        sessionKey[16];
} CharSelectAuth;

struct CharSelectThread {
    uint32_t            clientCount;
    uint32_t            authsAwaitingClientCount;
    uint32_t            clientsAwaitingAuthCount;
    int                 dbId;
    int                 logId;
    CharSelectClient**  clients;
    CharSelectAuth*     authsAwaitingClient;
    CharSelectClient**  clientsAwaitingAuth;
    uint64_t*           authAwaitingClientTimeouts;
    uint64_t*           clientAwaitingAuthTimeouts;
    RingBuf*            csQueue;
    RingBuf*            udpQueue;
    RingBuf*            dbQueue;
    RingBuf*            logQueue;
    /* Cached packets that are more or less static for all clients */
    TlgPacket*          packetLoginApproved;
    TlgPacket*          packetEnter;
    TlgPacket*          packetExpansionInfo;
    TlgPacket*          packetEcho1;
    TlgPacket*          packetEcho2;
    TlgPacket*          packetEcho3;
    TlgPacket*          packetEcho4;
    TlgPacket*          packetEcho5;
};

static bool cs_thread_drop_client(CharSelectThread* cs, CharSelectClient* client)
{
    ZPacket cmd;
    int rc;
    
    cmd.udp.zDropClient.ipAddress = csc_get_ip_addr(client);
    cmd.udp.zDropClient.packet = NULL;
    
    rc = ringbuf_push(cs->udpQueue, ZOP_UDP_DropClient, &cmd);
    
    if (rc)
    {
        uint32_t ip = cmd.udp.zDropClient.ipAddress.ip;
        
        log_writef(cs->logQueue, cs->logId, "cs_thread_drop_client: failed to inform UdpThread to drop client from %u.%u.%u.%u:%u, keeping client alive for now",
            (ip >> 0) & 0xff, (ip >> 8) & 0xff, (ip >> 16) & 0xff, (ip >> 24) & 0xff, cmd.udp.zDropClient.ipAddress.port);
    
        return false;
    }
    
    return true;
}

static int cs_thread_add_authed_client(CharSelectThread* cs, CharSelectClient* client)
{
    RingBuf* udpQueue = cs->udpQueue;
    uint32_t index = cs->clientCount;
    int rc;
    
    if (bit_is_pow2_or_zero(index))
    {
        uint32_t cap = (index == 0) ? 1 : index * 2;
        CharSelectClient** clients = realloc_array_type(cs->clients, cap, CharSelectClient*);
        
        if (!clients) goto fail;
        
        cs->clients = clients;
    }
    
    cs->clients[index] = client;
    cs->clientCount = index + 1;
    
    /* Info the client that we have accepted them */
    rc = csc_schedule_packet(client, udpQueue, cs->packetLoginApproved);
    if (rc) goto packet_fail;
    
    rc = csc_schedule_packet(client, udpQueue, cs->packetEnter);
    if (rc) goto packet_fail;
    
    rc = csc_schedule_packet(client, udpQueue, cs->packetExpansionInfo);
    if (rc) goto packet_fail;
    
    /*fixme: query the DB for client char select data*/
    
packet_fail:
    return rc;
    
fail:
    cs_thread_drop_client(cs, client);
    return ERR_OutOfMemory;
}

static bool cs_thread_is_auth_awaiting_client(CharSelectThread* cs, int64_t accountId, const char* sessionKey)
{
    CharSelectAuth* auths = cs->authsAwaitingClient;
    uint32_t n = cs->authsAwaitingClientCount;
    uint32_t i;
    
    for (i = 0; i < n; i++)
    {
        CharSelectAuth* auth = &auths[i];
        
        if (auth->accountId == accountId && memcmp(auth->sessionKey, sessionKey, sizeof(auth->sessionKey)) == 0)
        {
            /* Pop and swap */
            n--;
            auths[i] = auths[n];
            cs->authAwaitingClientTimeouts[i] = cs->authAwaitingClientTimeouts[n];
            cs->authsAwaitingClientCount = n;
            return true;
        }
    }
    
    return false;
}

static int cs_thread_add_client_awaiting_auth(CharSelectThread* cs, CharSelectClient* client)
{
    uint32_t index = cs->clientsAwaitingAuthCount;
    
    if (bit_is_pow2_or_zero(index))
    {
        uint32_t cap = (index == 0) ? 1 : index * 2;
        CharSelectClient** clients;
        uint64_t* timeouts;
        
        clients = realloc_array_type(cs->clientsAwaitingAuth, cap, CharSelectClient*);
        if (!clients) goto fail;
        cs->clientsAwaitingAuth = clients;
        
        timeouts = realloc_array_type(cs->clientAwaitingAuthTimeouts, cap, uint64_t);
        if (!timeouts) goto fail;
        cs->clientAwaitingAuthTimeouts = timeouts;
    }
    
    cs->clientsAwaitingAuth[index] = client;
    cs->clientAwaitingAuthTimeouts[index] = clock_milliseconds() + CHAR_SELECT_THREAD_AUTH_TIMEOUT_MS;
    cs->clientsAwaitingAuthCount = index + 1;
    return ERR_None;
    
fail:
    return ERR_OutOfMemory;
}

static int cs_thread_handle_op_login_info(CharSelectThread* cs, CharSelectClient* client, ZPacket* zpacket)
{
    Aligned a;
    const char* acct;
    int64_t accountId;
    const char* sessionKey;
    
    aligned_init(&a, zpacket->udp.zToServerPacket.data, zpacket->udp.zToServerPacket.length);
    
    if (aligned_remaining_data(&a) < 2)
        return ERR_Invalid;
    
    acct = aligned_current_type(&a, const char);
    
    if (aligned_advance_past_null_terminator(&a) < 4 || aligned_remaining_data(&a) < 16)
        return ERR_Invalid;
    
    accountId = strtol(acct + 3, NULL, 10); /* Skip "LS#", extract the digits */
    sessionKey = aligned_current_type(&a, const char);
    
    /* Check if this client is already authorized (highly likely) */
    if (cs_thread_is_auth_awaiting_client(cs, accountId, sessionKey))
        return cs_thread_add_authed_client(cs, client);
    
    /* Client wasn't authorized... add it to the clientsAwaitingAuth queue */
    return cs_thread_add_client_awaiting_auth(cs, client);
}

static int cs_thread_handle_op_echo(CharSelectThread* cs, CharSelectClient* client, uint16_t opcode)
{
    TlgPacket* packet;
    
    switch (opcode)
    {
    case OP_CS_Echo1:
        packet = cs->packetEcho1;
        break;
    
    case OP_CS_Echo2:
        packet = cs->packetEcho2;
        break;
    
    case OP_CS_Echo3:
        packet = cs->packetEcho3;
        break;
    
    case OP_CS_Echo4:
        packet = cs->packetEcho4;
        break;
    
    case OP_CS_Echo5:
        packet = cs->packetEcho5;
        break;
    }
    
    return csc_schedule_packet(client, cs->udpQueue, packet);
}

static void cs_thread_handle_packet(CharSelectThread* cs, ZPacket* zpacket)
{
    CharSelectClient* client = (CharSelectClient*)zpacket->udp.zToServerPacket.clientObject;
    byte* data = zpacket->udp.zToServerPacket.data;
    uint16_t opcode = zpacket->udp.zToServerPacket.opcode;
    int rc = ERR_None;
    
    switch (opcode)
    {
    case OP_CS_LoginInfo:
        rc = cs_thread_handle_op_login_info(cs, client, zpacket);
        break;
    
    case OP_CS_Echo1:
    case OP_CS_Echo2:
    case OP_CS_Echo3:
    case OP_CS_Echo4:
    case OP_CS_Echo5:
        rc = cs_thread_handle_op_echo(cs, client, opcode);
        break;
    
    default:
        log_writef(cs->logQueue, cs->logId, "WARNING: cs_thread_handle_packet: received unexpected char select opcode: %04x", zpacket->udp.zToServerPacket.opcode);
        break;
    }
    
    if (rc)
    {
        uint32_t ip = csc_get_ip(client);
        uint16_t port = csc_get_port(client);
        
        log_writef(cs->logQueue, cs->logId, "cs_thread_handle_packet: got error %s while processing packet with opcode %s from client from %u.%u.%u.%u:%u, disconnecting them to maintain consistency",
            enum2str_err(rc), enum2str_char_select_opcode(opcode), (ip >> 0) & 0xff, (ip >> 8) & 0xff, (ip >> 16) & 0xff, (ip >> 24) & 0xff, port);
        
        cs_thread_drop_client(cs, client);
    }
    
    if (data) free(data);
}

static void cs_thread_handle_login_auth(CharSelectThread* cs, ZPacket* zpacket)
{
    int64_t accountId = zpacket->cs.zLoginAuth.accountId;
    const char* sessionKey = zpacket->cs.zLoginAuth.sessionKey;
    CharSelectClient** clients = cs->clientsAwaitingAuth;
    uint32_t count = cs->clientsAwaitingAuthCount;
    uint32_t i;
    
    /* Check if we already have a client waiting for this auth (very unlikely) */
    for (i = 0; i < count; i++)
    {
        CharSelectClient* client = clients[i];
        
        if (csc_check_auth(client, accountId, sessionKey))
        {
            cs_thread_add_authed_client(cs, client);
            
            /* Pop and swap */
            count--;
            clients[i] = clients[count];
            cs->clientAwaitingAuthTimeouts[i] = cs->clientAwaitingAuthTimeouts[count];
            cs->clientsAwaitingAuthCount = count;
            return;
        }
    }
    
    count = cs->authsAwaitingClientCount;
    
    if (bit_is_pow2_or_zero(count))
    {
        uint32_t cap = (count == 0) ? 1 : count * 2;
        CharSelectAuth* auths;
        uint64_t* timeouts;
        
        auths = realloc_array_type(cs->authsAwaitingClient, cap, CharSelectAuth);
        if (!auths) goto fail;
        cs->authsAwaitingClient = auths;
        
        timeouts = realloc_array_type(cs->authAwaitingClientTimeouts, cap, uint64_t);
        if (!timeouts) goto fail;
        cs->authAwaitingClientTimeouts = timeouts;
    }
    
    cs->authsAwaitingClient[count].accountId = accountId;
    memcpy(cs->authsAwaitingClient[count].sessionKey, sessionKey, sizeof(cs->authsAwaitingClient[count].sessionKey));
    cs->authAwaitingClientTimeouts[count] = clock_milliseconds() + CHAR_SELECT_THREAD_AUTH_TIMEOUT_MS;
    cs->authsAwaitingClientCount = count + 1;
    
    log_writef(cs->logQueue, cs->logId, "Authorized account id %lli, awaiting matching client connection", accountId);
    return;
    
fail:
    log_writef(cs->logQueue, cs->logId, "cs_thread_handle_login_auth: out of memory while adding auth for account id %lli", accountId);
}

static void cs_thread_check_auth_timeouts(CharSelectThread* cs)
{
    uint64_t curTime = clock_milliseconds();
    CharSelectAuth* auths = cs->authsAwaitingClient;
    CharSelectClient** clients = cs->clientsAwaitingAuth;
    uint64_t* timeouts;
    uint32_t n, i;
    
    timeouts = cs->authAwaitingClientTimeouts;
    n = cs->authsAwaitingClientCount;
    i = 0;
    
    while (i < n)
    {
        if (timeouts[i] < curTime)
        {
            /* Pop and swap */
            n--;
            timeouts[i] = timeouts[n];
            auths[i] = auths[n];
            continue;
        }
        
        i++;
    }
    
    cs->authsAwaitingClientCount = n;
    
    timeouts = cs->clientAwaitingAuthTimeouts;
    n = cs->clientsAwaitingAuthCount;
    i = 0;
    
    while (i < n)
    {
        if (timeouts[i] < curTime)
        {
            cs_thread_drop_client(cs, clients[i]);
            
            /* Pop and swap */
            n--;
            timeouts[i] = timeouts[n];
            clients[i] = clients[n];
            continue;
        }
        
        i++;
    }
    
    cs->clientsAwaitingAuthCount = n;
}

static void cs_thread_remove_client_object(CharSelectThread* cs, CharSelectClient* client)
{
    if (client)
    {
        CharSelectClient** clients = cs->clients;
        uint32_t n = cs->clientCount;
        uint32_t i;
        
        for (i = 0; i < n; i++)
        {
            if (clients[i] == client)
            {
                /* Pop and swap */
                n--;
                clients[i] = clients[n];
                cs->clientCount = n;
                goto finish;
            }
        }
        
        clients = cs->clientsAwaitingAuth;
        n = cs->clientsAwaitingAuthCount;
        
        for (i = 0; i < n; i++)
        {
            if (clients[i] == client)
            {
                /* Pop and swap */
                n--;
                clients[i] = clients[n];
                cs->clientAwaitingAuthTimeouts[i] = cs->clientAwaitingAuthTimeouts[n];
                cs->clientsAwaitingAuthCount = n;
                goto finish;
            }
        }
        
    finish:
        csc_destroy(client);
    }
}

static void cs_thread_proc(void* ptr)
{
    CharSelectThread* cs = (CharSelectThread*)ptr;
    RingBuf* csQueue = cs->csQueue;
    ZPacket zpacket;
    int zop;
    int rc;
    
    for (;;)
    {
        /* The char select thread is purely packet-driven, always blocks until something needs to be done */
        rc = ringbuf_wait_for_packet(csQueue, &zop, &zpacket);
        
        if (rc)
        {
            log_writef(cs->logQueue, cs->logId, "cs_thread_proc: ringbuf_wait_for_packet() returned error: %s", enum2str_err(rc));
            break;
        }
        
        switch (zop)
        {
        case ZOP_UDP_ToServerPacket:
            cs_thread_handle_packet(cs, &zpacket);
            break;

        case ZOP_UDP_NewClient:
            csc_init(&zpacket);
            break;
        
        case ZOP_UDP_ClientLinkdead:
            break;
        
        case ZOP_UDP_ClientDisconnect:
            cs_thread_remove_client_object(cs, (CharSelectClient*)zpacket.udp.zClientDisconnect.clientObject);
            break;
        
        case ZOP_CS_TerminateThread:
            return;
        
        case ZOP_CS_LoginAuth:
            cs_thread_handle_login_auth(cs, &zpacket);
            break;
        
        case ZOP_CS_CheckAuthTimeouts:
            cs_thread_check_auth_timeouts(cs);
            break;
        
        default:
            log_writef(cs->logQueue, cs->logId, "WARNING: cs_thread_proc: received unexpected zop: %s", enum2str_zop(zop));
            break;
        }
    }
}

static int cs_create_cached_packets(CharSelectThread* cs)
{
    Aligned a;
    
    /* 1-byte packets, content always zero */
    cs->packetLoginApproved = packet_create(OP_CS_LoginApproved, 1);
    if (!cs->packetLoginApproved) goto fail;
    packet_grab(cs->packetLoginApproved);
    packet_data(cs->packetLoginApproved)[0] = 0;
    
    cs->packetEnter = packet_create(OP_CS_Enter, 1);
    if (!cs->packetEnter) goto fail;
    packet_grab(cs->packetEnter);
    packet_data(cs->packetEnter)[0] = 0;
    
    /* Expansion info */
    cs->packetExpansionInfo = packet_create_type(OP_CS_ExpansionInfo, uint32_t);
    if (!cs->packetExpansionInfo) goto fail;
    packet_grab(cs->packetExpansionInfo);
    
    aligned_init(&a, packet_data(cs->packetExpansionInfo), packet_length(cs->packetExpansionInfo));
    aligned_write_uint32(&a, 1 | 2); /* Bitfield: 1 = Kunark, 2 = Velious, 4 = Luclin */
    
    /* Zero-length echo packets */
    cs->packetEcho1 = packet_create_opcode_only(OP_CS_Echo1);
    if (!cs->packetEcho1) goto fail;
    packet_grab(cs->packetEcho1);
    
    cs->packetEcho2 = packet_create_opcode_only(OP_CS_Echo2);
    if (!cs->packetEcho2) goto fail;
    packet_grab(cs->packetEcho2);
    
    cs->packetEcho3 = packet_create_opcode_only(OP_CS_Echo3);
    if (!cs->packetEcho3) goto fail;
    packet_grab(cs->packetEcho3);
    
    cs->packetEcho4 = packet_create_opcode_only(OP_CS_Echo4);
    if (!cs->packetEcho4) goto fail;
    packet_grab(cs->packetEcho4);
    
    cs->packetEcho5 = packet_create_opcode_only(OP_CS_Echo5);
    if (!cs->packetEcho5) goto fail;
    packet_grab(cs->packetEcho5);
    
    return ERR_None;
fail:
    return ERR_OutOfMemory;
}

CharSelectThread* cs_create(LogThread* log, RingBuf* dbQueue, int dbId, UdpThread* udp)
{
    CharSelectThread* cs = alloc_type(CharSelectThread);
    int rc;
    
    if (!cs) goto fail_alloc;
    
    cs->clientCount = 0;
    cs->authsAwaitingClientCount = 0;
    cs->clientsAwaitingAuthCount = 0;
    cs->dbId = dbId;
    cs->clients = NULL;
    cs->authsAwaitingClient = NULL;
    cs->clientsAwaitingAuth = NULL;
    cs->authAwaitingClientTimeouts = NULL;
    cs->clientAwaitingAuthTimeouts = NULL;
    cs->csQueue = NULL;
    cs->udpQueue = udp_get_queue(udp);
    cs->dbQueue = dbQueue;
    cs->logQueue = log_get_queue(log);
    
    cs->packetLoginApproved = NULL;
    cs->packetEnter = NULL;
    cs->packetExpansionInfo = NULL;
    cs->packetEcho1 = NULL;
    cs->packetEcho2 = NULL;
    cs->packetEcho3 = NULL;
    cs->packetEcho4 = NULL;
    cs->packetEcho5 = NULL;
    
    rc = log_open_file_literal(log, &cs->logId, CHAR_SELECT_THREAD_LOG_PATH);
    if (rc) goto fail;

    cs->csQueue = ringbuf_create_type(ZPacket, 1024);
    if (!cs->csQueue) goto fail;
    
    rc = cs_create_cached_packets(cs);
    if (rc) goto fail;
    
    rc = udp_open_port(udp, CHAR_SELECT_THREAD_UDP_PORT, csc_sizeof(), cs->csQueue);
    if (rc) goto fail;

    rc = thread_start(cs_thread_proc, cs);
    if (rc) goto fail;
    
    return cs;
    
fail:
    cs_destroy(cs);
fail_alloc:
    return NULL;
}

CharSelectThread* cs_destroy(CharSelectThread* cs)
{
    if (cs)
    {
        cs->csQueue = ringbuf_destroy(cs->csQueue);
        
        cs->packetLoginApproved = packet_drop(cs->packetLoginApproved);
        cs->packetEnter = packet_drop(cs->packetEnter);
        cs->packetExpansionInfo = packet_drop(cs->packetExpansionInfo);
        cs->packetEcho1 = packet_drop(cs->packetEcho1);
        cs->packetEcho2 = packet_drop(cs->packetEcho2);
        cs->packetEcho3 = packet_drop(cs->packetEcho3);
        cs->packetEcho4 = packet_drop(cs->packetEcho4);
        cs->packetEcho5 = packet_drop(cs->packetEcho5);
        
        free(cs);
    }
    
    return NULL;
}

RingBuf* cs_get_queue(CharSelectThread* cs)
{
    return cs->csQueue;
}
