
#include "char_select_client.h"
#include "define_netcode.h"
#include "enum_zop.h"

struct CharSelectClient {
    IpAddr  ipAddress;
};

CharSelectClient* csc_init(ZPacket* zpacket)
{
    CharSelectClient* csc = (CharSelectClient*)zpacket->udp.zNewClient.clientObject;
    
    csc->ipAddress = zpacket->udp.zNewClient.ipAddress;

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
