
#include "client_packet_send.h"
#include "aligned.h"
#include "buffer.h"
#include "client.h"
#include "inventory.h"
#include "player_profile_packet_struct.h"
#include "skills.h"
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
    Inventory* inv = client_inv(client);
    uint64_t time = clock_milliseconds();
    uint32_t i;
    Aligned a;
    
    aligned_init(&a, &pp, sizeof(pp));
    
    /* skip crc for now, it gets written last */
    aligned_advance_by(&a, sizeof(uint32_t));
    /* name */
    aligned_write_snprintf_and_advance_by(&a, sizeof(pp.name), "%s", sbuf_str(client_name(client)));
    /* surname */
    aligned_write_snprintf_and_advance_by(&a, sizeof(pp.surname), "%s", client_surname_str_no_null(client));
    /* genderId */
    aligned_write_uint16(&a, client_base_gender_id(client));
    /* raceId */
    aligned_write_uint16(&a, client_base_race_id(client));
    /* classId */
    aligned_write_uint16(&a, client_class_id(client));
    /* level */
    aligned_write_uint32(&a, client_level(client));
    /* experience */
    aligned_write_uint32(&a, (uint32_t)client_experience(client));
    /* trainingPoints */
    aligned_write_uint16(&a, client_training_points(client));
    /* currentMana */
    aligned_write_int16(&a, (int16_t)client_cur_mana(client));
    /* faceId */
    aligned_write_uint8(&a, client_face_id(client));
    /* unknownA */
    aligned_write_zeroes(&a, sizeof(pp.unknownA));
    /* currentHp */
    aligned_write_int16(&a, (int16_t)client_cur_hp(client));
    /* unknownB */
    aligned_write_zeroes(&a, sizeof(pp.unknownB));
    /* STR */
    aligned_write_uint8(&a, client_base_str(client));
    /* STA */
    aligned_write_uint8(&a, client_base_sta(client));
    /* CHA */
    aligned_write_uint8(&a, client_base_cha(client));
    /* DEX */
    aligned_write_uint8(&a, client_base_dex(client));
    /* INT */
    aligned_write_uint8(&a, client_base_int(client));
    /* AGI */
    aligned_write_uint8(&a, client_base_agi(client));
    /* WIS */
    aligned_write_uint8(&a, client_base_wis(client));
    
    /* languages */
    skills_write_pp_languages(client_skills(client), &a);
    
    /* unknownC */
    aligned_write_zeroes(&a, sizeof(pp.unknownC));
    
    /* mainInventoryItemIds */
    inv_write_pp_main_item_ids(inv, &a);
    
    /* mainInventoryInternalUnused */
    aligned_write_zeroes(&a, sizeof(pp.mainInventoryInternalUnused));
    
    /* mainInventoryItemProperties */
    inv_write_pp_main_item_properties(inv, &a);
    
    /* buffs */
    /*fixme: do these properly once buffs are stored somewhere*/
    for (i = 0; i < ZEQ_PP_MAX_BUFFS; i++)
    {
        aligned_write_uint32(&a, 0);
        aligned_write_uint16(&a, 0xffff);   /* spellId */
        aligned_write_uint32(&a, 0);
    }
    
    /* baggedItemIds */
    inv_write_pp_main_bag_item_ids(inv, &a);
    
    /* baggedItemProperties */
    inv_write_pp_main_bag_item_properties(inv, &a);
    
    /* spellbook, memmedSpellIds */
    spellbook_write_pp(client_spellbook(client), &a);
    
    /* unknownD */
    aligned_write_zeroes(&a, sizeof(pp.unknownD));
    /* y */
    aligned_write_float(&a, client_loc_y(client));
    /* x */
    aligned_write_float(&a, client_loc_x(client));
    /* z */
    aligned_write_float(&a, client_loc_z(client));
    /* heading */
    aligned_write_float(&a, client_loc_heading(client));
    /* zoneShortName */
    aligned_write_snprintf_and_advance_by(&a, sizeof(pp.zoneShortName), "%s", zone_short_name(client_get_zone(client)));
    /* unknownEDefault100 */
    aligned_write_uint32(&a, 100);
    /* coins */
    aligned_write_buffer(&a, client_coin(client), sizeof(pp.coins));
    /* coinsBank */
    aligned_write_buffer(&a, client_coin_bank(client), sizeof(pp.coinsBank));
    /* coinsCursor */
    aligned_write_buffer(&a, client_coin_cursor(client), sizeof(pp.coinsCursor));
    
    /* skills */
    skills_write_pp(client_skills(client), &a);
    
    /* unknownF[162] */
    aligned_write_memset(&a, 0xff, 27);
    aligned_write_uint8(&a, 0);
    aligned_write_uint32(&a, 0xffffffff);
    aligned_write_uint8(&a, 0);
    aligned_write_memset(&a, 0xff, 3);
    aligned_write_uint8(&a, 0);
    aligned_write_uint8(&a, 0xff);
    aligned_write_uint8(&a, 0);
    aligned_write_memset(&a, 0xff, 12);
    aligned_write_uint16(&a, 0);
    aligned_write_uint8(&a, 0xff);
    aligned_write_uint16(&a, 0);
    aligned_write_uint8(&a, 0x80);
    aligned_write_uint8(&a, 0xbf);
    aligned_write_uint16(&a, 0);
    aligned_write_uint16(&a, 0x4040);
    aligned_write_uint16(&a, 0);
    aligned_write_uint8(&a, 0x20);
    aligned_write_uint8(&a, 0x40);
    aligned_write_uint16(&a, 0);
    aligned_write_uint8(&a, 0xb0);
    aligned_write_uint8(&a, 0x40);
    aligned_write_zeroes(&a, 92);
    
    /* autoSplit */
    
    /* pvpEnabled */
    
    /* unknownG */
    aligned_write_zeroes(&a, sizeof(pp.unknownG));
    /* isGM */
    
    /* disciplinesReady */
    
    /* unknownI */
    aligned_write_zeroes(&a, sizeof(pp.unknownI));
    /* hunger */
    
    /* thirst */
    
    /* unknownJ */
    aligned_write_zeroes(&a, sizeof(pp.unknownJ));
    
    /* bindZoneShortName[5] */
    
    cps_player_profile_compress_obfuscate_send(client, &pp);
}

void client_send_zone_entry(Client* client)
{
    
}
