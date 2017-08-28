
#include "char_select_client.h"
#include "define_netcode.h"
#include "udp_thread.h"
#include "util_ipv4.h"
#include "enum_zop.h"

struct CharSelectClient {
    bool        authed;
    bool        isNameApproved;
    bool        isLocal;
    bool        isZoning;
    IpAddr      ipAddress;
    int64_t     accountId;
    char        sessionKey[16];
};

CharSelectClient* csc_init(ZPacket* zpacket)
{
    CharSelectClient* csc = (CharSelectClient*)zpacket->udp.zNewClient.clientObject;
    
    csc->authed = false;
    csc->isNameApproved = false;
    csc->isLocal = ip_is_local(zpacket->udp.zNewClient.ipAddress.ip);
    csc->isZoning = false;
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

void csc_set_auth_data(CharSelectClient* csc, int64_t acctId, const char* sessionKey)
{
    csc->authed = true;
    csc->accountId = acctId;
    memcpy(csc->sessionKey, sessionKey, sizeof(csc->sessionKey));
}

bool csc_check_auth(CharSelectClient* csc, int64_t accountId, uint32_t ip, const char* sessionKey)
{
    /*fixme: accountId may be limited to 7 digits, should match the truncating behavior for this check*/
    return (csc->accountId == accountId && csc->ipAddress.ip == ip && memcmp(csc->sessionKey, sessionKey, sizeof(csc->sessionKey)) == 0);
}

bool csc_is_authed(CharSelectClient* csc)
{
    return csc->authed;
}

void csc_set_name_approved(CharSelectClient* csc, bool value)
{
    csc->isNameApproved = value;
}

bool csc_is_name_approved(CharSelectClient* csc)
{
    return csc->isNameApproved;
}

int64_t csc_get_account_id(CharSelectClient* csc)
{
    return csc->accountId;
}

bool csc_is_local(CharSelectClient* csc)
{
    return csc->isLocal;
}

void csc_set_is_zoning(CharSelectClient* csc, bool value)
{
    csc->isZoning = value;
}

bool csc_is_zoning(CharSelectClient* csc)
{
    return csc->isZoning;
}
