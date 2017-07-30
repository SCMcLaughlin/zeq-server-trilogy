
#include "login_crypto.h"
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/des.h>

static DES_key_schedule s_keySchedule;

void login_crypto_init_lib()
{
    DES_cblock iv = { 19, 217, 19, 109, 208, 52, 21, 251 };
    
    ERR_load_crypto_strings();
    OpenSSL_add_all_algorithms();
    CONF_modules_load_file(NULL, NULL, 0);
    
    DES_key_sched(&iv, &s_keySchedule);
}

void login_crypto_deinit_lib()
{
    EVP_cleanup();
    CRYPTO_cleanup_all_ex_data();
    CONF_modules_free();
    ERR_free_strings();
}

uint32_t login_crypto_process(const void* input, uint32_t length, void* output, int isEncrypt)
{
    DES_cblock iv = { 19, 217, 19, 109, 208, 52, 21, 251 };
    uint32_t rem = length % 8;
    
    if (rem)
        length += 8 - rem;
    
    DES_ncbc_encrypt((const byte*)input, (byte*)output, length, &s_keySchedule, &iv, isEncrypt);
    
    return length;
}

void login_crypto_hash(const char* password, uint32_t passlen, const byte* salt, uint32_t saltlen, void* output)
{
    PKCS5_PBKDF2_HMAC_SHA1(password, passlen, salt, saltlen, LOGIN_CRYPTO_HASH_ITERATIONS, LOGIN_CRYPTO_HASH_SIZE, (byte*)output);
}
