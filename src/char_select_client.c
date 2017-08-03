
#include "char_select_client.h"
#include "define_netcode.h"
#include "udp_thread.h"
#include "enum_zop.h"

struct CharSelectClient {
    bool        authed;
    bool        isNameApproved;
    IpAddr      ipAddress;
    int64_t     accountId;
    char        sessionKey[16];
    uint8_t     weaponMaterialIds[10][2];
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

void csc_set_auth_data(CharSelectClient* csc, int64_t acctId, const char* sessionKey)
{
    csc->authed = true;
    csc->accountId = acctId;
    memcpy(csc->sessionKey, sessionKey, sizeof(csc->sessionKey));
}

bool csc_check_auth(CharSelectClient* csc, int64_t accountId, const char* sessionKey)
{
    /*fixme: accountId may be limited to 7 digits, should match the truncating behavior for this check*/
    return (csc->accountId == accountId && memcmp(csc->sessionKey, sessionKey, sizeof(csc->sessionKey)) == 0);
}

bool csc_is_authed(CharSelectClient* csc)
{
    return csc->authed;
}

void csc_set_weapon_material_ids(CharSelectClient* csc, CharSelectData* data)
{
    /*fixme: make enum for the material ids */
    csc->weaponMaterialIds[0][0] = data->materialIds[0][7];
    csc->weaponMaterialIds[0][1] = data->materialIds[0][8];
    csc->weaponMaterialIds[1][0] = data->materialIds[1][7];
    csc->weaponMaterialIds[1][1] = data->materialIds[1][8];
    csc->weaponMaterialIds[2][0] = data->materialIds[2][7];
    csc->weaponMaterialIds[2][1] = data->materialIds[2][8];
    csc->weaponMaterialIds[3][0] = data->materialIds[3][7];
    csc->weaponMaterialIds[3][1] = data->materialIds[3][8];
    csc->weaponMaterialIds[4][0] = data->materialIds[4][7];
    csc->weaponMaterialIds[4][1] = data->materialIds[4][8];
    csc->weaponMaterialIds[5][0] = data->materialIds[5][7];
    csc->weaponMaterialIds[5][1] = data->materialIds[5][8];
    csc->weaponMaterialIds[6][0] = data->materialIds[6][7];
    csc->weaponMaterialIds[6][1] = data->materialIds[6][8];
    csc->weaponMaterialIds[7][0] = data->materialIds[7][7];
    csc->weaponMaterialIds[7][1] = data->materialIds[7][8];
    csc->weaponMaterialIds[8][0] = data->materialIds[8][7];
    csc->weaponMaterialIds[8][1] = data->materialIds[8][8];
    csc->weaponMaterialIds[9][0] = data->materialIds[9][7];
    csc->weaponMaterialIds[9][1] = data->materialIds[9][8];
}

uint8_t csc_get_weapon_material_id(CharSelectClient* csc, uint32_t index, uint32_t slot)
{
    return csc->weaponMaterialIds[index][slot - 7];
}

void csc_set_name_approved(CharSelectClient* csc, bool value)
{
    csc->isNameApproved = value;
}

bool csc_is_name_approved(CharSelectClient* csc)
{
    return csc->isNameApproved;
}
