
#include "udp_thread.h"
#include "bit.h"
#include "udp_client.h"
#include "util_alloc.h"
#include "util_clock.h"
#include "util_thread.h"
#include "zpacket.h"
#include "enum_zop.h"
#include "enum2str.h"

#define UDP_THREAD_BUFFER_SIZE 1024
#define UDP_THREAD_MINIMUM_MEANINGFUL_PACKET_SIZE 10
#define UDP_THREAD_LOG_PATH "log/udp_thread.txt"
#define UDP_THREAD_LINKDEAD_TIMEOUT_MS 15000
#define UDP_THREAD_KEEP_ALIVE_ACK_DELAY_MS 500

enum UdpClientFlag
{
    UDP_FLAG_ReadyToSend,
    UDP_FLAG_IgnorePackets,
    UDP_FLAG_Dead,
};

typedef struct {
    sock_t      sock;
    uint32_t    clientSize;
    RingBuf*    toServerQueueDefault;
} UdpSocket;

struct UdpThread {
    uint32_t    sockCount;
    uint32_t    clientCount;
    UdpSocket*  sockets;
    IpAddr*     clientAddresses;
    UdpClient*  clients;
    uint8_t*    clientFlags;
    uint64_t*   clientRecvTimestamps;
    uint64_t*   clientLastAckTimestamps;
    RingBuf*    udpQueue;
    RingBuf*    statusQueue;
    RingBuf*    logQueue;
    int         logId;
    byte        recvBuffer[UDP_THREAD_BUFFER_SIZE];
};

static bool udp_thread_open_socket(UdpThread* udp, sock_t* outSock, uint16_t port)
{
    IpAddrRaw addr;
    sock_t sock;
#ifdef PLATFORM_WINDOWS
    unsigned long nonblock = 1;
#endif

    sock = socket(AF_INET, SOCK_DGRAM, 0);

    if (sock == INVALID_SOCKET)
    {
        log_write_literal(udp->logQueue, udp->logId, "ERROR: udp_thread_open_socket: socket() syscall failed");
        return false;
    }

    /* Set non-blocking */
#ifdef PLATFORM_WINDOWS
    if (ioctlsocket(sock, FIONBIO, &nonblock))
#else
    if (fcntl(sock, F_SETFL, O_NONBLOCK))
#endif
    {
        log_writef(udp->logQueue, udp->logId, "ERROR: udp_thread_open_socket: failed to set non-blocking mode for socket %i", sock);
        goto fail_open;
    }

    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = to_network_uint16(port);
    addr.sin_addr.s_addr = to_network_uint32(INADDR_ANY);

    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)))
    {
        log_writef(udp->logQueue, udp->logId, "ERROR: udp_thread_open_socket: bind() syscall failed for socket %i on port %u", sock, port);
        goto fail_open;
    }

    log_writef(udp->logQueue, udp->logId, "Listening for UDP packets with socket %i on port %u", sock, port);
    *outSock = sock;
    return true;
fail_open:
    closesocket(sock);
    return false;
}

static void udp_thread_open_port(UdpThread* udp, ZPacket* cmd)
{
    sock_t sock = INVALID_SOCKET;
    UdpSocket* usock;
    uint32_t index;

    if (!udp_thread_open_socket(udp, &sock, cmd->udp.zOpenPort.port))
        return;

    index = udp->sockCount;

    if (bit_is_pow2_or_zero(index))
    {
        uint32_t cap = (index == 0) ? 1 : index * 2;
        UdpSocket* array = realloc_array_type(udp->sockets, cap, UdpSocket);

        if (!array)
        {
            log_writef(udp->logQueue, udp->logId, "ERROR: udp_thread_open_port: out of memory while attempting to create new UdpSocket object for port %u", cmd->udp.zOpenPort.port);
            return;
        }

        udp->sockets = array;
    }

    udp->sockCount = index + 1;
    usock = &udp->sockets[index];
    usock->sock = sock;
    usock->clientSize = cmd->udp.zOpenPort.clientSize;
    usock->toServerQueueDefault = cmd->udp.zOpenPort.toServerQueue;
}

static UdpClient* udp_thread_get_client_by_ip(UdpThread* udp, uint32_t ip, uint16_t port, uint32_t* cliIndex)
{
    UdpClient* clients = udp->clients;
    IpAddr* addresses = udp->clientAddresses;
    uint32_t n = udp->clientCount;
    uint32_t i;
    
    /*fixme: consider replacing with a 64bit hash lookup (benchmarking needed to determine if it would be worth it at typical client counts)*/
    for (i = 0; i < n; i++)
    {
        IpAddr* addr = &addresses[i];
        
        if (addr->ip != ip || addr->port != port)
            continue;
        
        if (cliIndex)
            *cliIndex = i;
        
        return &clients[i];
    }
    
    return NULL;
}

static void udp_thread_handle_client_packet(UdpThread* udp, ZPacket* cmd, bool ackRequest)
{
    uint32_t index;
    UdpClient* client = udp_thread_get_client_by_ip(udp, cmd->udp.zToClientPacket.ipAddress.ip, cmd->udp.zToClientPacket.ipAddress.port, &index);
    
    if (client)
    {
        udpc_schedule_packet(client, cmd->udp.zToClientPacket.packet, ackRequest);
        bit_set(udp->clientFlags[index], UDP_FLAG_ReadyToSend);
    }
}

static void udp_thread_handle_drop_client(UdpThread* udp, ZPacket* cmd)
{
    uint32_t index;
    UdpClient* client = udp_thread_get_client_by_ip(udp, cmd->udp.zToClientPacket.ipAddress.ip, cmd->udp.zToClientPacket.ipAddress.port, &index);
    
    if (client)
    {
        bit_set(udp->clientFlags[index], UDP_FLAG_IgnorePackets);
    }
}

static void udp_thread_handle_replace_client_object(UdpThread* udp, ZPacket* cmd)
{
    UdpClient* client = udp_thread_get_client_by_ip(udp, cmd->udp.zReplaceClientObject.ipAddress.ip, cmd->udp.zReplaceClientObject.ipAddress.port, NULL);
    
    if (client)
    {
        udpc_replace_client_object(udp, client, cmd->udp.zReplaceClientObject.clientObject);
    }
}

static bool udp_thread_process_commands(UdpThread* udp)
{
    RingBuf* cmdQueue = udp->udpQueue;
    ZPacket cmd;
    int zop;
    int rc;

    for (;;)
    {
        rc = ringbuf_pop(cmdQueue, &zop, &cmd);
        if (rc) break;

        switch (zop)
        {
        case ZOP_UDP_TerminateThread:
            return false;

        case ZOP_UDP_OpenPort:
            udp_thread_open_port(udp, &cmd);
            break;
        
        case ZOP_UDP_ToClientPacketScheduled:
            udp_thread_handle_client_packet(udp, &cmd, true);
            break;
        
        case ZOP_UDP_DropClient:
            udp_thread_handle_drop_client(udp, &cmd);
            break;
        
        case ZOP_UDP_ReplaceClientObject:
            udp_thread_handle_replace_client_object(udp, &cmd);
            break;

        default:
            log_writef(udp->logQueue, udp->logId, "WARNING: udp_thread_process_commands: received unexpected zop: %s", enum2str_zop(zop));
            break;
        }
    }

    return true;
}

static int udp_thread_new_client(UdpThread* udp, uint32_t clientSize, UdpClient* toCopy)
{
    void* clientObject = alloc_bytes(clientSize);
    ZPacket zpacket;
    uint32_t index;
    uint32_t ip = toCopy->ipAddr.ip;
    uint64_t time;

    if (!clientObject) goto oom;

    index = udp->clientCount;

    if (bit_is_pow2_or_zero(index))
    {
        uint32_t cap = (index == 0) ? 1 : index * 2;
        UdpClient* clients;
        IpAddr* clientAddresses;
        uint64_t* clientRecvTimestamps;
        uint64_t* clientLastAckTimestamps;
        uint8_t* clientFlags;

        clients = realloc_array_type(udp->clients, cap, UdpClient);
        if (!clients) goto oom;
        udp->clients = clients;
        
        clientFlags = realloc_array_type(udp->clientFlags, cap, uint8_t);
        if (!clientFlags) goto oom;
        udp->clientFlags = clientFlags;

        clientAddresses = realloc_array_type(udp->clientAddresses, cap, IpAddr);
        if (!clientAddresses) goto oom;
        udp->clientAddresses = clientAddresses;

        clientRecvTimestamps = realloc_array_type(udp->clientRecvTimestamps, cap, uint64_t);
        if (!clientRecvTimestamps) goto oom;
        udp->clientRecvTimestamps = clientRecvTimestamps;

        clientLastAckTimestamps = realloc_array_type(udp->clientLastAckTimestamps, cap, uint64_t);
        if (!clientLastAckTimestamps) goto oom;
        udp->clientLastAckTimestamps = clientLastAckTimestamps;
    }

    zpacket.udp.zNewClient.clientObject = clientObject;
    zpacket.udp.zNewClient.ipAddress = toCopy->ipAddr;
    
    udpc_set_index(toCopy, index);
    toCopy->clientObject = clientObject;

    if (ringbuf_push(toCopy->toServerQueue, ZOP_UDP_NewClient, &zpacket))
    {
        log_write_literal(udp->logQueue, udp->logId, "udp_thread_new_client: ringbuf_push() failed for ZOP_UDP_NewClient");
        goto queue_fail;
    }
    
    log_writef(udp->logQueue, udp->logId, "New UDP client connection from %u.%u.%u.%u:%u",
        (ip >> 0) & 0xff, (ip >> 8) & 0xff, (ip >> 16) & 0xff, (ip >> 24) & 0xff, to_host_uint16(toCopy->ipAddr.port));
    
    time = clock_milliseconds();

    udp->clientCount = index + 1;
    udp->clients[index] = *toCopy;
    udp->clientFlags[index] = 0;
    udp->clientAddresses[index] = zpacket.udp.zNewClient.ipAddress;
    udp->clientRecvTimestamps[index] = time;
    udp->clientLastAckTimestamps[index] = time;
    return (int)index;
oom:
    log_writef(udp->logQueue, udp->logId, "ERROR: udp_thread_new_client: out of memory while allocating client from %u.%u.%u.%u:%u",
        (ip >> 0) & 0xff, (ip >> 8) & 0xff, (ip >> 16) & 0xff, (ip >> 24) & 0xff, to_host_uint16(toCopy->ipAddr.port));

queue_fail:
    if (clientObject) free(clientObject);
    return -1;
}

static void udp_thread_dump_packet_recv(byte* data, int len)
{
#ifndef ZEQ_UDP_DUMP_PACKETS
    (void)data;
    (void)len;
#else
    int i;
    
    fprintf(stdout, "[Recv] (%i):\n", len);
    
    for (i = 0; i < len; i++)
    {
        fprintf(stdout, "%02x ", data[i]);
    }
    
    fputc('\n', stdout);
    fflush(stdout);
#endif
}

static void udp_thread_process_socket_reads(UdpThread* udp)
{
    IpAddrRaw raddr;
    socklen_t addrlen = sizeof(IpAddrRaw);
    UdpSocket* sockets = udp->sockets;
    byte* buffer = udp->recvBuffer;
    uint32_t n = udp->sockCount;
    uint32_t i;
    
    for (i = 0; i < n; i++)
    {
        sock_t sock = sockets[i].sock;
        
        for (;;)
        {
            UdpClient* client;
            uint32_t ip, port, cliIndex;
            int len;
            
            len = recvfrom(sock, (char*)buffer, UDP_THREAD_BUFFER_SIZE, 0, (struct sockaddr*)&raddr, &addrlen);
            
            if (len < 0)
            {
            #ifdef PLATFORM_WINDOWS
                int err = WSAGetLastError();
                if (err != WSAEWOULDBLOCK && err != WSAECONNRESET)
            #else
                int err = errno;
                if (err != EAGAIN && err != EWOULDBLOCK)
            #endif
                {
                    log_writef(udp->logQueue, udp->logId, "WARNING: udp_thread_process_socket_reads: recvfrom() syscall had unexpected errcode: %i", err);
                }
                
                break;
            }
            
            if (len < UDP_THREAD_MINIMUM_MEANINGFUL_PACKET_SIZE)
                continue;
            
            udp_thread_dump_packet_recv(buffer, len);
            
            /* Determine if we already know about this client connection */
            ip = raddr.sin_addr.s_addr;
            port = raddr.sin_port;
            client = udp_thread_get_client_by_ip(udp, ip, port, &cliIndex);
            
            if (client)
            {
                udp->clientRecvTimestamps[cliIndex] = clock_milliseconds();

                if (!bit_get(udp->clientFlags[cliIndex], UDP_FLAG_IgnorePackets))
                {
                    if (udpc_recv_protocol(udp, client, buffer, (uint32_t)len, false))
                        bit_set(udp->clientFlags[cliIndex], UDP_FLAG_ReadyToSend);
                }
                
                continue;
            }

            /* Scope for temporary on-stack UdpClient */
            {
                UdpClient newClient;

                udpc_init(&newClient, sock, ip, port, sockets[i].toServerQueueDefault);

                /* This will return true if the packet seems valid and is not flagged as session-terminating */
                if (udpc_recv_protocol(udp, &newClient, buffer, (uint32_t)len, true))
                {
                    int index = udp_thread_new_client(udp, sockets[i].clientSize, &newClient);
                    /*
                        We redo this because we want to avoid telling a thread they've received a packet from a new client before telling them that new client exists.
                        We could allocate the client object first instead and avoid all of this, but we want to avoid doing an allocation for invalid connections sending garbage data.
                    */
                    if (index != -1)
                        udpc_recv_protocol(udp, &udp->clients[index], buffer, (uint32_t)len, false);
                }
            }
        }
    }
}

static void udp_thread_process_socket_writes(UdpThread* udp)
{
    UdpClient* clients = udp->clients;
    uint8_t* clientFlags = udp->clientFlags;
    uint32_t count = udp->clientCount;
    uint32_t i;
    
    for (i = 0; i < count; i++)
    {
        if (bit_get(clientFlags[i], UDP_FLAG_ReadyToSend) == 0)
            continue;
        
        bit_unset(clientFlags[i], UDP_FLAG_ReadyToSend);
        udpc_send_queued_packets(udp, &clients[i]);
    }
}

static void udp_thread_check_client_timeouts(UdpThread* udp)
{
    UdpClient* clients = udp->clients;
    uint8_t* clientFlags = udp->clientFlags;
    uint64_t* clientRecvTimestamps = udp->clientRecvTimestamps;
    uint32_t count = udp->clientCount;
    uint64_t time = clock_milliseconds();
    uint32_t i;
    
    for (i = 0; i < count; i++)
    {
        if ((time - clientRecvTimestamps[i]) >= UDP_THREAD_LINKDEAD_TIMEOUT_MS)
        {
            bit_set(clientFlags[i], UDP_FLAG_Dead);
            udpc_linkdead(udp, &clients[i]);
        }
    }
}

static void udp_thread_send_keep_alive_acks(UdpThread* udp)
{
    UdpClient* clients = udp->clients;
    uint8_t* clientFlags = udp->clientFlags;
    uint64_t* clientLastAckTimestamps = udp->clientLastAckTimestamps;
    uint32_t count = udp->clientCount;
    uint64_t time = clock_milliseconds();
    uint32_t i;

    for (i = 0; i < count; i++)
    {
        if ((time - clientLastAckTimestamps[i]) >= UDP_THREAD_KEEP_ALIVE_ACK_DELAY_MS && (clientFlags[i] & (nth_bit(UDP_FLAG_IgnorePackets) | nth_bit(UDP_FLAG_Dead))) == 0)
        {
            clientLastAckTimestamps[i] = time;
            udpc_send_keep_alive_ack(&clients[i]);
        }
    }
}

static void udp_thread_cleanup_dead_clients(UdpThread* udp)
{
    IpAddr* clientAddresses = udp->clientAddresses;
    UdpClient* clients = udp->clients;
    uint8_t* clientFlags = udp->clientFlags;
    uint64_t* clientRecvTimestamps = udp->clientRecvTimestamps;
    uint64_t* clientLastAckTimestamps = udp->clientLastAckTimestamps;
    uint32_t count = udp->clientCount;
    uint32_t i = 0;
    
    while (i < count)
    {
        if (bit_get(clientFlags[i], UDP_FLAG_Dead))
        {
            UdpClient* client = &clients[i];
            
            udpc_send_disconnect(udp, client);
            udpc_deinit(client);
            
            /* Pop and swap */
            count--;
            clients[i] = clients[count];
            udpc_set_index(&clients[i], i);
            clientFlags[i] = clientFlags[count];
            clientAddresses[i] = clientAddresses[count];
            clientRecvTimestamps[i] = clientRecvTimestamps[count];
            clientLastAckTimestamps[i] = clientLastAckTimestamps[count];
            continue;
        }
        
        i++;
    }
    
    udp->clientCount = count;
}

static void udp_thread_proc(void* ptr)
{
    UdpThread* udp = (UdpThread*)ptr;

    for (;;)
    {
        udp_thread_process_socket_reads(udp);

        if (!udp_thread_process_commands(udp))
            break;

        udp_thread_process_socket_writes(udp);
        
        udp_thread_check_client_timeouts(udp);
        udp_thread_cleanup_dead_clients(udp);
        udp_thread_send_keep_alive_acks(udp);

        clock_sleep(25);
    }
}

UdpThread* udp_create(LogThread* log)
{
    UdpThread* udp = alloc_type(UdpThread);
    int rc;

    if (!udp) goto fail_alloc;

    udp->sockCount = 0;
    udp->clientCount = 0;
    udp->sockets = NULL;
    udp->clientAddresses = NULL;
    udp->clients = NULL;
    udp->clientFlags = NULL;
    udp->clientRecvTimestamps = NULL;
    udp->clientLastAckTimestamps = NULL;
    udp->udpQueue = NULL;
    udp->statusQueue = NULL;
    udp->logQueue = log_get_queue(log);

    rc = log_open_file_literal(log, &udp->logId, UDP_THREAD_LOG_PATH);
    if (rc) goto fail;

    udp->udpQueue = ringbuf_create_type(ZPacket, 1024);
    if (!udp->udpQueue) goto fail;

    rc = thread_start(udp_thread_proc, udp);
    if (rc) goto fail;

    return udp;

fail:
    udp_destroy(udp);
fail_alloc:
    return NULL;
}

static void udp_free_clients(UdpThread* udp)
{
    UdpClient* clients = udp->clients;
    
    if (clients)
    {
        uint32_t count = udp->clientCount;
        uint32_t i;
        
        for (i = 0; i < count; i++)
        {
            udpc_deinit(&clients[i]);
        }
        
        free(clients);
        udp->clients = NULL;
        udp->clientCount = 0;
    }
}

static void udp_free_sockets(UdpThread* udp)
{
    UdpSocket* sockets = udp->sockets;
    
    if (sockets)
    {
        uint32_t count = udp->sockCount;
        uint32_t i;
        
        for (i = 0; i < count; i++)
        {
            UdpSocket* usock = &sockets[i];
            
            closesocket(usock->sock);
            usock->sock = INVALID_SOCKET;
        }
        
        free(sockets);
        udp->sockets = NULL;
        udp->sockCount = 0;
    }
}

UdpThread* udp_destroy(UdpThread* udp)
{
    if (udp)
    {
        udp_free_sockets(udp);
        udp_free_clients(udp);
        free_if_exists(udp->clientAddresses);
        free_if_exists(udp->clientFlags);
        free_if_exists(udp->clientRecvTimestamps);
        free_if_exists(udp->clientLastAckTimestamps);
        ringbuf_destroy_if_exists(udp->udpQueue);
        ringbuf_destroy_if_exists(udp->statusQueue);
        log_close_file(udp->logQueue, udp->logId);
        free(udp);
    }

    return NULL;
}

int udp_trigger_shutdown(UdpThread* udp)
{
    if (udp->udpQueue)
    {
        return ringbuf_push(udp->udpQueue, ZOP_UDP_TerminateThread, NULL);
    }

    return ERR_None;
}

int udp_open_port(UdpThread* udp, uint16_t port, uint32_t clientSize, RingBuf* toServerQueue)
{
    ZPacket cmd;

    cmd.udp.zOpenPort.port = port;
    cmd.udp.zOpenPort.clientSize = clientSize;
    cmd.udp.zOpenPort.toServerQueue = toServerQueue;

    return ringbuf_push(udp->udpQueue, ZOP_UDP_OpenPort, &cmd);
}

RingBuf* udp_get_queue(UdpThread* udp)
{
    return udp->udpQueue;
}

int udp_schedule_packet(RingBuf* toClientQueue, IpAddr ipAddr, TlgPacket* packet)
{
    ZPacket cmd;
    int rc;

    cmd.udp.zToClientPacket.ipAddress = ipAddr;
    cmd.udp.zToClientPacket.packet = packet;
    
    if (!packet_already_fragmentized(packet))
        packet_fragmentize(packet);
    
    packet_grab(packet);
    rc = ringbuf_push(toClientQueue, ZOP_UDP_ToClientPacketScheduled, &cmd);
    if (rc) packet_drop(packet);

    return rc;
}

RingBuf* udp_get_log_queue(UdpThread* udp)
{
    return udp->logQueue;
}

int udp_get_log_id(UdpThread* udp)
{
    return udp->logId;
}

void udp_thread_update_ack_timestamp(UdpThread* udp, uint32_t index)
{
    udp->clientLastAckTimestamps[index] = clock_milliseconds();
}
