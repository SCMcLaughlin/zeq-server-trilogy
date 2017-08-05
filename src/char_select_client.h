
#ifndef CHAR_SELECT_CLIENT_H
#define CHAR_SELECT_CLIENT_H

#include "define.h"
#include "buffer.h"
#include "char_select_data.h"
#include "ringbuf.h"
#include "tlg_packet.h"
#include "zpacket.h"

typedef struct CharSelectClient CharSelectClient;

CharSelectClient* csc_init(ZPacket* zpacket);
CharSelectClient* csc_destroy(CharSelectClient* csc);

uint32_t csc_sizeof();

int csc_schedule_packet(CharSelectClient* csc, RingBuf* toClientQueue, TlgPacket* packet);

IpAddr csc_get_ip_addr(CharSelectClient* csc);
uint32_t csc_get_ip(CharSelectClient* csc);
uint16_t csc_get_port(CharSelectClient* csc);

void csc_set_auth_data(CharSelectClient* csc, int64_t acctId, const char* sessionKey);
bool csc_check_auth(CharSelectClient* csc, int64_t accountId, const char* sessionKey);
bool csc_is_authed(CharSelectClient* csc);

void csc_set_weapon_material_ids(CharSelectClient* csc, CharSelectData* data);
uint8_t csc_get_weapon_material_id(CharSelectClient* csc, uint32_t index, uint32_t slot);

void csc_set_name_approved(CharSelectClient* csc, bool value);
bool csc_is_name_approved(CharSelectClient* csc);

int64_t csc_get_account_id(CharSelectClient* csc);
bool csc_is_local(CharSelectClient* csc);

#endif/*CHAR_SELECT_CLIENT_H*/
