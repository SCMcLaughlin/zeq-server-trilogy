
#include "client_packet_send.h"
#include "aligned.h"
#include "client.h"
#include "player_profile_packet_struct.h"
#include "tlg_packet.h"
#include "udp_thread.h"
#include "udp_zpacket.h"
#include "util_clock.h"
#include "zone.h"
#include "enum_opcode.h"
#include <zlib.h>

static void cps_obfuscate_player_profile(byte* data, uint32_t len)
{
    uint32_t* ptr = (uint32_t*)data;
    uint32_t cur = 0x65e7;
    uint32_t n = len / sizeof(uint32_t);
    uint32_t next;
    uint32_t i;
    
    i = len / 8;
    next = ptr[0];
    ptr[0] = ptr[i];
    ptr[i] = next;
    
    for (i = 0; i < n; i++)
    {
        next = cur + ptr[i] - 0x37a9;
        ptr[i] = ((ptr[i] << 0x07) | (ptr[i] >> 0x19)) + 0x37a9;
        ptr[i] = ((ptr[i] << 0x0f) | (ptr[i] >> 0x11));
        ptr[i] = ptr[i] - cur;
        cur = next;
    }
}

static void cps_player_profile_compress_obfuscate_send(Client* client, PS_PlayerProfile* pp)
{
    byte buffer[sizeof(PS_PlayerProfile)];
    unsigned long length = sizeof(buffer);
    TlgPacket* packet;
    int rc;
    
    rc = compress2(buffer, &length, (const byte*)pp, sizeof(PS_PlayerProfile), Z_BEST_COMPRESSION);
    
    if (rc != Z_OK)
    {
        Zone* zone = client_get_zone(client);
        log_writef(zone_log_queue(zone), zone_log_id(zone), "ERROR: cps_player_profile_compress_obfuscate_send: compress2() failed, errcode: %i", rc);
        return;
    }
    
    cps_obfuscate_player_profile(buffer, (uint32_t)length);
    
    packet = packet_create(OP_PlayerProfile, (uint32_t)length);
    
    if (packet)
    {
        memcpy(packet_data(packet), buffer, length);
        client_schedule_packet(client, packet);
    }
}

static void cps_obfuscate_spawn(byte* data, uint32_t len)
{
    uint32_t* ptr = (uint32_t*)data;
    uint32_t cur = 0;
    uint32_t n = len / sizeof(uint32_t);
    uint32_t next;
    uint32_t i;
    
    for (i = 0; i < n; i++)
    {
        next = cur + ptr[i] - 0x65e7;
        ptr[i] = ((ptr[i] << 0x09) | (ptr[i] >> 0x17)) + 0x65e7;
        ptr[i] = ((ptr[i] << 0x0d) | (ptr[i] >> 0x13));
        ptr[i] = ptr[i] - cur;
        cur = next;
    }
}

void client_schedule_packet(Client* client, TlgPacket* packet)
{
    Zone* zone = client_get_zone(client);
    
    udp_schedule_packet(zone_udp_queue(zone), client_ip_addr(client), packet);
}

void client_send_echo_copy(Client* client, ToServerPacket* packet)
{
    uint32_t len = packet->length;
    byte* data = packet->data;
    TlgPacket* copy = packet_create(packet->opcode, len);
    
    if (len && data)
        memcpy(packet_data(copy), data, len);
    
    client_schedule_packet(client, copy);
}

void client_send_player_profile(Client* client)
{
    PS_PlayerProfile pp;
    uint64_t time = clock_milliseconds();
    uint32_t reset;
    Aligned a;
    
    aligned_init(&a, &pp, sizeof(pp));
    
    
    cps_player_profile_compress_obfuscate_send(client, &pp);
}

void client_send_zone_entry(Client* client)
{
    
}
