
#ifndef UTIL_RANDOM_H
#define UTIL_RANDOM_H

#include "define.h"

void random_bytes(void* buffer, int count);
uint32_t random_uint32();
uint16_t random_uint16();
uint8_t random_uint8();

#endif/*UTIL_RANDOM_H*/
