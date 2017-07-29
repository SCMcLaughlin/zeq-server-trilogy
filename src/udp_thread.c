
#include "udp_thread.h"
#include "define_netcode.h"
#include "bit.h"
#include "udp_client.h"
#include "util_alloc.h"
#include "util_clock.h"
#include "util_thread.h"
#include "zpacket.h"
#include "enum_zop.h"

#define UDP_THREAD_BUFFER_SIZE 1024
#define UDP_THREAD_MINIMUM_MEANINGFUL_PACKET_SIZE 10
#define UDP_THREAD_LOG_PATH "log/udp_thread.txt"

typedef struct {
    sock_t      sock;
    uint32_t    clientSize;
    RingBuf*    toServerQueueDefault;
    RingBuf*    toClientQueue;
} UdpSocket;

struct UdpThread {
    uint32_t    sockCount;
    uint32_t    clientCount;
    UdpSocket*  sockets;
    IpAddr*     clientAddresses;
    UdpClient*  clients;
    uint64_t*   clientRecvTimestamps;
    RingBuf*    commandQueue;
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
        log_write_literal(udp->logQueue, udp->logId, "udp_thread_open_socket: socket() syscall failed");
        return false;
    }

    /* Set non-blocking */
#ifdef PLATFORM_WINDOWS
    if (ioctlsocket(sock, FIONBIO, &nonblock))
#else
    if (fcntl(sock, F_SETFL, O_NONBLOCK))
#endif
    {
        log_writef(udp->logQueue, udp->logId, "udp_thread_open_socket: failed to set non-blocking mode for socket %i", sock);
        goto fail_open;
    }

    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = to_network_uint16(port);
    addr.sin_addr.s_addr = to_network_uint32(INADDR_ANY);

    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)))
    {
        log_writef(udp->logQueue, udp->logId, "udp_thread_open_socket: bind() syscall failed for socket %i on port %u", sock, port);
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

    if (udp_thread_open_socket(udp, &sock, cmd->udp.zOpenPort.port))
        return;

    index = udp->sockCount;

    if (bit_is_pow2_or_zero(index))
    {
        uint32_t cap = (index == 0) ? 1 : index * 2;
        UdpSocket* array = realloc_array_type(udp->sockets, cap, UdpSocket);

        if (!array)
        {
            log_writef(udp->logQueue, udp->logId, "udp_thread_open_port: out of memory while attempting to create new UdpSocket object for port %u", cmd->udp.zOpenPort.port);
            return;
        }

        udp->sockets = array;
    }

    udp->sockCount = index + 1;
    usock = &udp->sockets[index];
    usock->sock = sock;
    usock->clientSize = cmd->udp.zOpenPort.clientSize;
    usock->toServerQueueDefault = cmd->udp.zOpenPort.toServerQueue;
    usock->toClientQueue = cmd->udp.zOpenPort.toClientQueue;
}

static bool udp_thread_process_commands(UdpThread* udp)
{
    RingBuf* cmdQueue = udp->commandQueue;
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

        default:
            break;
        }
    }

    return true;
}

static void udp_thread_new_client(UdpThread* udp, uint32_t clientSize, UdpClient* toCopy)
{
    void* clientObject = alloc_bytes(clientSize);
    ZPacket zpacket;
    uint32_t index;
    uint32_t ip;

    if (!clientObject) goto oom;

    index = udp->clientCount;

    if (bit_is_pow2_or_zero(index))
    {
        uint32_t cap = (index == 0) ? 1 : index * 2;
        UdpClient* clients;
        IpAddr* clientAddresses;
        uint64_t* clientRecvTimestamps;

        clients = realloc_array_type(udp->clients, cap, UdpClient);
        if (!clients) goto oom;
        udp->clients = clients;

        clientAddresses = realloc_array_type(udp->clientAddresses, cap, IpAddr);
        if (!clientAddresses) goto oom;
        udp->clientAddresses = clientAddresses;

        clientRecvTimestamps = realloc_array_type(udp->clientRecvTimestamps, cap, uint64_t);
        if (!clientRecvTimestamps) goto oom;
        udp->clientRecvTimestamps = clientRecvTimestamps;
    }

    zpacket.udp.zNewClient.clientObject = clientObject;
    zpacket.udp.zNewClient.ipAddress = toCopy->ipAddr;

    if (ringbuf_push(toCopy->toServerQueue, ZOP_UDP_NewClient, &zpacket))
    {
        log_write_literal(udp->logQueue, udp->logId, "udp_thread_new_client: ringbuf_push() failed for ZOP_UDP_NewClient");
        goto queue_fail;
    }
    
    udp->clientCount = index + 1;
    udp->clients[index] = *toCopy;
    udp->clients[index].clientObject = clientObject;
    udp->clientAddresses[index] = zpacket.udp.zNewClient.ipAddress;
    udp->clientRecvTimestamps[index] = clock_milliseconds();
    return;
oom:
    ip = toCopy->ipAddr.ip;
    log_writef(udp->logQueue, udp->logId, "udp_thread_new_client: out of memory while allocating client from %u.%u.%u.%u:%u",
        (ip >> 0) & 0xff, (ip >> 8) & 0xff, (ip >> 16) & 0xff, (ip >> 24) & 0xff, toCopy->ipAddr.port);

queue_fail:
    if (clientObject) free(clientObject);
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
            IpAddr* addrArray;
            UdpClient* client;
            uint32_t ip, port, cliIndex, m;
            int len;
            
            len = recvfrom(sock, (char*)buffer, UDP_THREAD_BUFFER_SIZE, 0, (struct sockaddr*)&raddr, &addrlen);
            
            if (len < 0)
            {
            #ifdef PLATFORM_WINDOWS
                int err = WSAGetLastError();
                if (err != WSAEWOULDBLOCK)
            #else
                int err = errno;
                if (err != EAGAIN && err != EWOULDBLOCK)
            #endif
                {
                    log_writef(udp->logQueue, udp->logId, "udp_thread_process_socket_reads: recvfrom() syscall had unexpected errcode: %i", err);
                }
                
                break;
            }
            
            if (len < UDP_THREAD_MINIMUM_MEANINGFUL_PACKET_SIZE)
                continue;
            
            /* Determine if we already know about this client connection */
            ip = raddr.sin_addr.s_addr;
            port = raddr.sin_port;
            client = NULL;
            addrArray = udp->clientAddresses;
            m = udp->clientCount;
            
            for (cliIndex = 0; cliIndex < m; cliIndex++)
            {
                IpAddr* addr = &addrArray[cliIndex];
                
                /*fixme: consider replacing with a 64bit hash lookup (benchmarking needed to determine if it would be worth it at typical client counts)*/
                if (addr->ip == ip && addr->port == port)
                {
                    client = &udp->clients[cliIndex];

                    if (udpc_recv_protocol(client, buffer, (uint32_t)len, false))
                    {
                        udp->clientRecvTimestamps[cliIndex] = clock_milliseconds();
                    }

                    goto next_packet;
                }
            }

            /* Scope for temporary on-stack UdpClient */
            {
                UdpClient newClient;

                udpc_init(&newClient, sock, ip, port, sockets[i].toServerQueueDefault, udp->logQueue);

                /* This will return true if the packet seems valid and is not flagged as session-terminating */
                if (udpc_recv_protocol(&newClient, buffer, (uint32_t)len, true))
                {
                    udp_thread_new_client(udp, sockets[i].clientSize, &newClient);
                    /*
                        We redo this because we want to avoid telling a thread they've received a packet from a new client before telling them that new client exists.
                        We could allocate the client object first instead and avoid all of this, but we want to avoid doing an allocation for invalid connections sending garbage data.
                    */
                    udpc_recv_protocol(&newClient, buffer, (uint32_t)len, false);
                }
            }

        next_packet:;
        }
    }
}

static void udp_thread_process_socket_writes(UdpThread* udp)
{

}

static void udp_thread_proc(void* ptr)
{
    UdpThread* udp = (UdpThread*)ptr;

    for (;;)
    {
        if (!udp_thread_process_commands(udp))
            break;

        udp_thread_process_socket_reads(udp);
        udp_thread_process_socket_writes(udp);

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
    udp->clientRecvTimestamps = NULL;
    udp->commandQueue = NULL;
    udp->statusQueue = NULL;
    udp->logQueue = log_get_queue(log);

    rc = log_open_file_literal(log, &udp->logId, UDP_THREAD_LOG_PATH);
    if (rc) goto fail;

    udp->commandQueue = ringbuf_create_type(ZPacket, 8);
    if (!udp->commandQueue) goto fail;

    udp->statusQueue = ringbuf_create_no_content(256);
    if (!udp->statusQueue) goto fail;

    rc = thread_start(udp_thread_proc, udp);
    if (rc) goto fail;

    return udp;

fail:
    udp_destroy(udp);
fail_alloc:
    return NULL;
}

UdpThread* udp_destroy(UdpThread* udp)
{
    if (udp)
    {
        //fixme: todo: free sockets and clients properly
        free_if_exists(udp->clientAddresses);
        free_if_exists(udp->clientRecvTimestamps);
        ringbuf_destroy_if_exists(udp->commandQueue);
        ringbuf_destroy_if_exists(udp->statusQueue);
        log_close_file(udp->logQueue, udp->logId);
        free(udp);
    }

    return NULL;
}

int udp_trigger_shutdown(UdpThread* udp)
{
    if (udp->commandQueue)
    {
        return ringbuf_push(udp->commandQueue, ZOP_UDP_TerminateThread, NULL);
    }

    return ERR_None;
}

int udp_open_port(UdpThread* udp, uint16_t port, uint32_t clientSize, RingBuf* toServerQueue, RingBuf* toClientQueue)
{
    ZPacket cmd;

    cmd.udp.zOpenPort.port = port;
    cmd.udp.zOpenPort.clientSize = clientSize;
    cmd.udp.zOpenPort.toServerQueue = toServerQueue;
    cmd.udp.zOpenPort.toClientQueue = toClientQueue;

    return ringbuf_push(udp->commandQueue, ZOP_UDP_OpenPort, &cmd);
}
