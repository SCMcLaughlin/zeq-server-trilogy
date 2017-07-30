
#include "login_client.h"
#include "define_netcode.h"
#include "aligned.h"
#include "buffer.h"
#include "login_packet_struct.h"
#include "udp_zpacket.h"
#include "util_ipv4.h"
#include "util_random.h"
#include "util_str.h"
#include "enum_login_opcode.h"
#include "enum_zop.h"

struct LoginClient {
    int8_t          authState;
    bool            isLocal;
    IpAddr          ipAddress;
    int64_t         accountId;
    StaticBuffer*   accountName;
    StaticBuffer*   passwordTemp;
};

enum LoginAuthState
{
    LOGIN_AUTH_Initial,
    LOGIN_AUTH_VersionSent,
    LOGIN_AUTH_ProcessingCredentials,
    LOGIN_AUTH_Authorized,
};

LoginClient* loginc_init(ZPacket* zpacket)
{
    LoginClient* loginc = (LoginClient*)zpacket->udp.zNewClient.clientObject;

    loginc->authState = LOGIN_AUTH_Initial;
    loginc->isLocal = ip_is_local(zpacket->udp.zNewClient.ipAddress.ip);
    loginc->ipAddress = zpacket->udp.zNewClient.ipAddress;
    loginc->accountId = 0;
    loginc->accountName = NULL;
    loginc->passwordTemp = NULL;

    return loginc;
}

LoginClient* loginc_destroy(LoginClient* loginc)
{
    if (loginc)
    {
        loginc->accountName = sbuf_drop(loginc->accountName);
        loginc_clear_password(loginc);
        free(loginc);
    }

    return NULL;
}

uint32_t loginc_sizeof()
{
    return sizeof(LoginClient);
}

int loginc_schedule_packet(LoginClient* loginc, RingBuf* toClientQueue, TlgPacket* packet)
{
    ZPacket cmd;
    int rc;

    cmd.udp.zToClientPacket.ipAddress = loginc->ipAddress;
    cmd.udp.zToClientPacket.packet = packet;

    packet_grab(packet);
    rc = ringbuf_push(toClientQueue, ZOP_UDP_ToClientPacketScheduled, &cmd);
    if (rc) packet_drop(packet);

    return rc;
}

int loginc_handle_op_version(LoginClient* loginc, RingBuf* toClientQueue, TlgPacket* packet)
{
    int rc = loginc_schedule_packet(loginc, toClientQueue, packet);

    if (rc == ERR_None)
    {
        if (loginc->authState < LOGIN_AUTH_VersionSent)
        {
            loginc->authState = LOGIN_AUTH_VersionSent;
        }
    }

    return rc;
}

int loginc_handle_op_credentials(LoginClient* loginc, ToServerPacket* packet)
{
    PSLogin_Credentials* ptr;
    byte decrypt[64];
    int namelen;
    int passlen;
    int rc = ERR_Invalid;
    
    /*fixme: look into making this a static assert?*/
    assert(sizeof(decrypt) > (sizeof(PSLogin_Credentials) + 8));
    
    if (loginc->authState < LOGIN_AUTH_VersionSent || packet->length < sizeof(PSLogin_Credentials))
        goto invalid;
    
    /* The client spams this packet while we are dealing with it... only do work for the first request */
    if (loginc->authState == LOGIN_AUTH_ProcessingCredentials)
        return ERR_Again;
    
    /* Decrypt the username and password */
    login_crypto_process(packet->data, packet->length, decrypt, false);
    
    ptr = (PSLogin_Credentials*)decrypt;
    
    namelen = str_len_overflow(ptr->username, sizeof_field(PSLogin_Credentials, username) - 1);
    passlen = str_len_overflow(ptr->password, sizeof_field(PSLogin_Credentials, password) - 1);
    
    if (namelen <= 0 || passlen <= 0)
        goto invalid_decrypted;
    
    /* Ensure accountName is not set */
    loginc->accountName = sbuf_drop(loginc->accountName);
    loginc->accountName = sbuf_create(ptr->username, (uint32_t)namelen);
    sbuf_grab(loginc->accountName);
    
    if (!loginc->accountName)
    {
        rc = ERR_OutOfMemory;
        goto invalid_decrypted;
    }
    
    loginc_clear_password(loginc);
    loginc->passwordTemp = sbuf_create(ptr->password, (uint32_t)passlen);
    sbuf_grab(loginc->passwordTemp);

    if (!loginc->passwordTemp)
    {
        rc = ERR_OutOfMemory;
        goto invalid_decrypted;
    }
    
    loginc->authState = LOGIN_AUTH_ProcessingCredentials;
    rc = ERR_None;
    
invalid_decrypted:
    /* Don't let the decrypted data linger on the stack */
    memset(decrypt, 0, sizeof(decrypt));
invalid:
    return rc;
}

int loginc_handle_db_credentials(LoginClient* loginc, ZPacket* zpacket, RingBuf* toClientQueue, TlgPacket* errPacket)
{
    byte hash[LOGIN_CRYPTO_HASH_SIZE];
    StaticBuffer* password;
    byte* salt;
    
    if (loginc->authState != LOGIN_AUTH_ProcessingCredentials)
        goto cred_err;
        
    password = loginc->passwordTemp;
    salt = zpacket->db.zResult.rLoginCredentials.salt;
    
    /* Check if the password hash matches */
    login_crypto_hash(sbuf_str(password), sbuf_length(password), salt, LOGIN_CRYPTO_SALT_SIZE, hash);
    
    if (memcmp(hash, zpacket->db.zResult.rLoginCredentials.passwordHash, LOGIN_CRYPTO_HASH_SIZE) != 0)
    {
    cred_err:
        loginc_schedule_packet(loginc, toClientQueue, errPacket);
        return ERR_Mismatch;
    }
    
    /* Successful login, send our response */
    loginc_clear_password(loginc);
    return loginc_send_session_packet(loginc, toClientQueue, zpacket->db.zResult.rLoginCredentials.acctId);
}

int loginc_send_session_packet(LoginClient* loginc, RingBuf* toClientQueue, int64_t acctId)
{
    TlgPacket* packet;
    Aligned a;
    
    packet = packet_create_type(OP_LOGIN_Session, PSLogin_Session);
    if (!packet) return ERR_OutOfMemory;
    
    loginc->accountId = acctId;
    loginc->authState = LOGIN_AUTH_Authorized;
    
    aligned_init(&a, packet_data(packet), packet_length(packet));
    /* sessionId */
    aligned_write_snprintf_and_advance_by(&a, sizeof_field(PSLogin_Session, sessionId), "LS#%i", acctId);
    /* "unused" */
    aligned_write_literal_null_terminated(&a, "unused");
    /* unknown */
    aligned_write_uint32(&a, 4);

    return loginc_schedule_packet(loginc, toClientQueue, packet);
}

int loginc_handle_op_banner(LoginClient* loginc, RingBuf* toClientQueue, TlgPacket* packet)
{
    if (loginc->authState < LOGIN_AUTH_Authorized)
        return ERR_Invalid;
    
    return loginc_schedule_packet(loginc, toClientQueue, packet);
}

IpAddr loginc_get_ip_addr(LoginClient* loginc)
{
    return loginc->ipAddress;
}

uint32_t loginc_get_ip(LoginClient* loginc)
{
    return loginc->ipAddress.ip;
}

uint16_t loginc_get_port(LoginClient* loginc)
{
    return loginc->ipAddress.port;
}

StaticBuffer* loginc_get_account_name(LoginClient* loginc)
{
    return loginc->accountName;
}

StaticBuffer* loginc_get_password(LoginClient* loginc)
{
    return loginc->passwordTemp;
}

void loginc_clear_password(LoginClient* loginc)
{
    sbuf_zero_fill(loginc->passwordTemp);
    loginc->passwordTemp = sbuf_drop(loginc->passwordTemp);
}
