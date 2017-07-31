
#include "char_select_client.h"
#include "define_netcode.h"
#include "udp_thread.h"
#include "enum_zop.h"

struct CharSelectClient {
    bool        authed;
    IpAddr      ipAddress;
    int64_t     accountId;
    char        sessionKey[16];
};

CharSelectClient* csc_init(ZPacket* zpacket)
{
    CharSelectClient* csc = (CharSelectClient*)zpacket->udp.zNewClient.clientObject;
    
    csc->authed = false;
    csc->ipAddress = zpacket->udp.zNewClient.ipAddress;
    csc->accountId = -1;

    return csc;
}

CharSelectClient* csc_destroy(CharSelectClient* csc)
{
    if (csc)
    {
        free(csc);
    }

    return NULL;
}

uint32_t csc_sizeof()
{
    return sizeof(CharSelectClient);
}

int csc_schedule_packet(CharSelectClient* csc, RingBuf* toClientQueue, TlgPacket* packet)
{
    return udp_schedule_packet(toClientQueue, csc->ipAddress, packet);
}

IpAddr csc_get_ip_addr(CharSelectClient* csc)
{
    return csc->ipAddress;
}

uint32_t csc_get_ip(CharSelectClient* csc)
{
    return csc->ipAddress.ip;
}

uint16_t csc_get_port(CharSelectClient* csc)
{
    return csc->ipAddress.port;
}

bool csc_check_auth(CharSelectClient* csc, int64_t accountId, const char* sessionKey)
{
    if (csc->accountId == accountId && memcmp(csc->sessionKey, sessionKey, sizeof(csc->sessionKey)) == 0)
    {
        csc->authed = true;
        return true;
    }
    
    return false;
}
