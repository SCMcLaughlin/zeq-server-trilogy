
#ifndef ACK_MGR_H
#define ACK_MGR_H

#include "define.h"
#include "aligned.h"
#include "tlg_packet.h"

struct UdpThread;
struct UdpClient;

typedef struct {
    uint16_t    ackRequest;
    uint16_t    fragCount;
    uint16_t    opcode;
    uint32_t    length;
    byte*       data;
} AckMgrToServerPacket;

typedef struct {
    uint16_t                nextAckRequestExpected;
    uint16_t                lastAckSent;
    uint16_t                lastAckReceived;
    uint16_t                lowestAckInQueue;
    uint16_t                count;
    uint16_t                capacity;
    AckMgrToServerPacket*   packetQueue;
} AckMgrToServer;

typedef struct {
    uint16_t    header;
    uint16_t    ackRequest;
    uint16_t    fragGroup;
    uint16_t    fragCount;
    uint8_t     ackCounterAlwaysOne;
    uint8_t     ackCounterRequest;
    uint64_t    ackTimestamp;
    TlgPacket*  packet;
} AckMgrToClientPacket;

typedef struct {
    uint16_t                nextSeqToSend;
    uint16_t                nextAckToRequest;
    uint16_t                nextFragGroup;
    uint8_t                 ackCounterAlwaysOne; /* If this turns out to NOT always be one, then this field will have a point; otherwise, remove later */
    uint8_t                 ackCounterRequest;
    uint32_t                sendFromIndex;
    uint32_t                count;
    AckMgrToClientPacket*   packetQueue;
} AckMgrToClient;

typedef struct {
    AckMgrToServer  toServer;
    AckMgrToClient  toClient;
} AckMgr;

void ack_init(AckMgr* ackMgr);
void ack_deinit(AckMgr* ackMgr);

void ack_schedule_packet(AckMgr* ackMgr, TlgPacket* packet, bool hasAckRequest);

void ack_recv_ack_response(AckMgr* ackMgr, uint16_t ack);
void ack_recv_sync(AckMgr* ackMgr, uint16_t ackRequest);
void ack_recv_packet(struct UdpThread* udp, struct UdpClient* udpc, Aligned* a, uint16_t opcode, uint16_t ackRequest, uint16_t fragCount);

void ack_send_queued_packets(struct UdpThread* udp, struct UdpClient* udpc);

uint16_t ack_get_next_seq_to_send_and_increment(AckMgr* ackMgr);
uint16_t ack_get_keep_alive_ack(AckMgr* ackMgr);

#endif/*ACK_MGR_H*/
