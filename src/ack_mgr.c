
#include "ack_mgr.h"
#include "define_netcode.h"

#define ACK_CMP_WINDOW_HALF_SIZE 1024

enum AckComparison
{
    ACK_Past,
    ACK_Present,
    ACK_Future,
};

static int ack_cmp(uint16_t got, uint16_t expected)
{
    if (got == expected)
        return ACK_Present;

    if ((got > expected && got < (expected + ACK_CMP_WINDOW_HALF_SIZE)) || got < (expected - ACK_CMP_WINDOW_HALF_SIZE))
        return ACK_Future;

    return ACK_Past;
}

void ack_init(AckMgr* ackMgr)
{
    memset(ackMgr, 0, sizeof(AckMgr));
}

void ack_deinit(AckMgr* ackMgr)
{
    uint32_t n, i;
    
    if (ackMgr->toServer.packetQueue)
    {
        AckMgrToServerPacket* packets = ackMgr->toServer.packetQueue;
        n = ackMgr->toServer.count;
        
        for (i = 0; i < n; i++)
        {
            AckMgrToServerPacket* p = &packets[i];
            
            if (p->data)
            {
                free(p->data);
                p->data = NULL;
            }
        }
        
        free(packets);
        ackMgr->toServer.packetQueue = NULL;
    }
    
    if (ackMgr->toClient.packetQueue)
    {
        AckMgrToClientPacket* packets = ackMgr->toClient.packetQueue;
        n = ackMgr->toClient.count;
        
        for (i = 0; i < n; i++)
        {
            packets[i].packet = packet_drop(packets[i].packet);
        }
        
        free(packets);
        ackMgr->toClient.packetQueue = NULL;
    }
}

void ack_recv_ack_response(AckMgr* ackMgr, uint16_t ack)
{
    AckMgrToClientPacket* packets = ackMgr->toClient.packetQueue;
    uint32_t n = ackMgr->toClient.count;
    uint32_t i;

    ack = to_host_uint16(ack);
    ackMgr->toClient.lastAckReceived = ack;

    for (i = 0; i < n; i++)
    {
        AckMgrToClientPacket* packet = &packets[i];
        uint16_t ackRequest = packet->ackRequest + (packet->fragCount - 1);

        if (ack_cmp(ackRequest, ack) == ACK_Future)
            break;

        packet->packet = packet_drop(packet->packet);
    }

    if (i > 0)
    {
        n -= i;
        memmove(packets, &packets[i], sizeof(AckMgrToClientPacket) * n);
        ackMgr->toClient.count = n;
        ackMgr->toClient.sendFromIndex = 0;
    }
}

void ack_recv_ack_request(AckMgr* ackMgr, uint16_t ack, int isFirstPacket)
{
    if (!isFirstPacket)
    {
        if (ack == (ackMgr->toClient.nextAckRequestEnd + 1))
        {
            ackMgr->toClient.nextAckResponse = ack;
            ackMgr->toClient.nextAckRequestEnd = ack;
        }
    }
    else
    {
        ackMgr->toClient.nextAckResponse = ack;
        ackMgr->toClient.nextAckRequestStart = ack;
        ackMgr->toClient.nextAckRequestEnd = ack;
    }
}
