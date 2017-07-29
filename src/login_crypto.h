
#ifndef LOGIN_CRYPTO_H
#define LOGIN_CRYPTO_H

#include "define.h"

#define LOGIN_CRYPTO_HASH_SIZE 16
#define LOGIN_CRYPTO_HASH_ITERATIONS 100000
#define LOGIN_CRYPTO_SALT_SIZE 4

void login_crypto_init_lib();
void login_crypto_deinit_lib();

uint32_t login_crypto_process(const void* input, uint32_t length, void* output, int isEncrypt);
void login_crypto_hash(const char* password, uint32_t passlen, const byte* salt, uint32_t saltlen, void* output);

#endif/*LOGIN_CRYPTO_H*/
