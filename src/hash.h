
#ifndef HASH_H
#define HASH_H

#include "define.h"

uint32_t hash_int64(int64_t key);
uint32_t hash_str(const char* key, uint32_t len);

#endif/*HASH_H*/
