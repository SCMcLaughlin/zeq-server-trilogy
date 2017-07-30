
#ifndef LOGIN_CLIENT_H
#define LOGIN_CLIENT_H

#include "define.h"
#include "ringbuf.h"
#include "tlg_packet.h"
#include "zpacket.h"

typedef struct LoginClient LoginClient;

LoginClient* loginc_init(ZPacket* zpacket);
LoginClient* loginc_destroy(LoginClient* loginc);

uint32_t loginc_sizeof();

int loginc_schedule_packet(LoginClient* loginc, RingBuf* toClientQueue, TlgPacket* packet);
int loginc_handle_op_version(LoginClient* loginc, RingBuf* toClientQueue, TlgPacket* packet);
int loginc_handle_op_credentials(LoginClient* loginc, ToServerPacket* packet);
int loginc_handle_db_credentials(LoginClient* loginc, ZPacket* zpacket, RingBuf* toClientQueue, TlgPacket* errPacket);
int loginc_handle_op_banner(LoginClient* loginc, RingBuf* toClientQueue, TlgPacket* packet);

IpAddr loginc_get_ip_addr(LoginClient* loginc);
uint32_t loginc_get_ip(LoginClient* loginc);
uint16_t loginc_get_port(LoginClient* loginc);

StaticBuffer* loginc_get_account_name(LoginClient* loginc);
StaticBuffer* loginc_get_password(LoginClient* loginc);
void loginc_clear_password(LoginClient* loginc);

#endif/*LOGIN_CLIENT_H*/
