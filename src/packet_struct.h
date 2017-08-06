
#ifndef PACKET_STRUCT_H
#define PACKET_STRUCT_H

#include "define.h"

#pragma pack(1)

typedef struct {
    uint32_t    checksum;
    char        name[32];
} PS_ZoneEntryClient;

#pragma pack()

#endif/*PACKET_STRUCT_H*/
