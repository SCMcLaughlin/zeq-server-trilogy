
#include "client_packet_send.h"
#include "aligned.h"
#include "buffer.h"
#include "client.h"
#include "crc.h"
#include "inventory.h"
#include "packet_create.h"
#include "packet_struct.h"
#include "player_profile_packet_struct.h"
#include "skills.h"
#include "tlg_packet.h"
#include "udp_thread.h"
#include "udp_zpacket.h"
#include "util_clock.h"
#include "zone.h"
#include "zone_id.h"
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

void client_schedule_packet(Client* client, TlgPacket* packet)
{
    client_schedule_packet_with_zone(client, client_get_zone(client), packet);
}

void client_schedule_packet_with_zone(Client* client, Zone* zone, TlgPacket* packet)
{
    client_schedule_packet_with_udp_queue(client, zone_udp_queue(zone), packet);
}

void client_schedule_packet_with_udp_queue(Client* client, RingBuf* udpQueue, TlgPacket* packet)
{
    udp_schedule_packet(udpQueue, client_ip_addr(client), packet);
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
    BindPoint* bind = client_bind_point(client, 0);
    uint64_t time = clock_milliseconds();
    uint64_t time2;
    uint32_t i;
    Aligned a;
    
    aligned_init(&a, &pp, sizeof(pp));
    
    /* skip crc for now, it gets written last */
    aligned_advance_by(&a, sizeof(uint32_t));
    /* name */
    aligned_write_snprintf_and_advance_by(&a, sizeof(pp.name), "%s", client_name_str(client));
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
    aligned_write_uint32(&a, client_is_auto_split_enabled(client));
    /* pvpEnabled */
    aligned_write_uint32(&a, client_is_pvp(client));
    /* unknownG */
    aligned_write_zeroes(&a, sizeof(pp.unknownG));
    /* isGM */
    aligned_write_uint32(&a, client_is_gm(client));
    /* unknownH */
    aligned_write_zeroes(&a, sizeof(pp.unknownH));
    /* disciplinesReady */
    aligned_write_uint32(&a, (time >= client_disc_timestamp(client)));
    /* unknownI */
    aligned_write_zeroes(&a, sizeof(pp.unknownI));
    /* hunger */
    aligned_write_uint32(&a, client_hunger(client));
    /* thirst */
    aligned_write_uint32(&a, client_thirst(client));
    /* unknownJ */
    aligned_write_zeroes(&a, sizeof(pp.unknownJ));
    
    /* bindZoneShortName[5] */
    aligned_write_snprintf_and_advance_by(&a, sizeof(pp.bindZoneShortName[0]), "%s", zone_short_name_by_id(bind[0].zoneId));
    aligned_write_snprintf_and_advance_by(&a, sizeof(pp.bindZoneShortName[1]), "%s", zone_short_name_by_id(bind[1].zoneId));
    aligned_write_snprintf_and_advance_by(&a, sizeof(pp.bindZoneShortName[2]), "%s", zone_short_name_by_id(bind[2].zoneId));
    aligned_write_snprintf_and_advance_by(&a, sizeof(pp.bindZoneShortName[3]), "%s", zone_short_name_by_id(bind[3].zoneId));
    aligned_write_snprintf_and_advance_by(&a, sizeof(pp.bindZoneShortName[4]), "%s", zone_short_name_by_id(bind[4].zoneId));
    
    /* bankInventoryItemProperties */
    inv_write_pp_bank_item_properties(inv, &a);
    
    /* bankBaggedItemProperties */
    inv_write_pp_bank_bag_item_properties(inv, &a);
    
    /* unknownK, unknownL */
    aligned_write_zeroes(&a, sizeof(pp.unknownK) + sizeof(pp.unknownL));
    
    /* bindLocY[5] */
    aligned_write_float(&a, bind[0].loc.y);
    aligned_write_float(&a, bind[1].loc.y);
    aligned_write_float(&a, bind[2].loc.y);
    aligned_write_float(&a, bind[3].loc.y);
    aligned_write_float(&a, bind[4].loc.y);
    /* bindLocX[5] */
    aligned_write_float(&a, bind[0].loc.x);
    aligned_write_float(&a, bind[1].loc.x);
    aligned_write_float(&a, bind[2].loc.x);
    aligned_write_float(&a, bind[3].loc.x);
    aligned_write_float(&a, bind[4].loc.x);
    /* bindLocZ[5] */
    aligned_write_float(&a, bind[0].loc.z);
    aligned_write_float(&a, bind[1].loc.z);
    aligned_write_float(&a, bind[2].loc.z);
    aligned_write_float(&a, bind[3].loc.z);
    aligned_write_float(&a, bind[4].loc.z);
    /* bindLocHeading[5] */
    aligned_write_float(&a, bind[0].loc.heading);
    aligned_write_float(&a, bind[1].loc.heading);
    aligned_write_float(&a, bind[2].loc.heading);
    aligned_write_float(&a, bind[3].loc.heading);
    aligned_write_float(&a, bind[4].loc.heading);
    
    /* bankInventoryInternalUnused, unknownM */
    aligned_write_zeroes(&a, sizeof(pp.bankInventoryInternalUnused) + sizeof(pp.unknownM));
    /* unixTimeA */
    aligned_write_uint32(&a, (uint32_t)client_creation_timestamp(client)); /*fixme: is this supposed to be creation time, or current time?*/
    /* unknownN */
    aligned_write_zeroes(&a, sizeof(pp.unknownN));
    /* unknownODefault1 */
    aligned_write_uint32(&a, 1);
    /* unknownP */
    aligned_write_zeroes(&a, sizeof(pp.unknownP));
    
    /* bankInventoryItemIds */
    inv_write_pp_bank_item_ids(inv, &a);
    
    /* bankBaggedItemIds */
    inv_write_pp_bank_bag_item_ids(inv, &a);
    
    /* deity */
    aligned_write_uint16(&a, client_deity_id(client));
    /* guildId */
    aligned_write_uint16(&a, client_guild_id_or_ffff(client));
    /* unixTimeB */
    aligned_write_uint32(&a, (uint32_t)time);
    /* unknownQ */
    aligned_write_zeroes(&a, sizeof(pp.unknownQ));
    /* unknownRDefault7f7f */
    aligned_write_uint16(&a, 0x7f7f);
    /* fatiguePercent */
    aligned_write_uint8(&a, 0); /*fixme: handle this*/
    /* unknownS */
    aligned_write_zeroes(&a, sizeof(pp.unknownS));
    /* unknownTDefault1 */
    aligned_write_uint8(&a, 1);
    /* anon */
    aligned_write_uint16(&a, client_anon_setting(client));
    /* guildRank */
    aligned_write_uint8(&a, client_guild_rank_or_ff(client));
    /* drunkeness */
    aligned_write_uint8(&a, client_drunkeness(client));
    /* showEqLoadScreen, unknownU */
    aligned_write_zeroes(&a, sizeof(pp.showEqLoadScreen) + sizeof(pp.unknownU));
    
    /* spellGemRefreshMilliseconds */
    spellbook_write_pp_gem_refresh(client_spellbook(client), &a, time);
    
    /* unknownV */
    aligned_write_zeroes(&a, sizeof(pp.unknownV));
    
    /* harmtouchRefreshMilliseconds */
    time2 = client_harmtouch_timestamp(client);
    aligned_write_uint32(&a, (uint32_t)((time >= time2) ? 0 : time2 - time));
    
    /* groupMemberNames */
    /*fixme: handle these */
    aligned_write_zeroes(&a, sizeof(pp.groupMember[0]));
    aligned_write_zeroes(&a, sizeof(pp.groupMember[1]));
    aligned_write_zeroes(&a, sizeof(pp.groupMember[2]));
    aligned_write_zeroes(&a, sizeof(pp.groupMember[3]));
    aligned_write_zeroes(&a, sizeof(pp.groupMember[4]));
    
    /* unknownW */
    aligned_write_zeroes(&a, 70);
    aligned_write_uint16(&a, 0xffff);
    aligned_write_zeroes(&a, 6);
    aligned_write_memset(&a, 0xff, 6);
    aligned_write_zeroes(&a, 176);
    aligned_write_uint16(&a, 0xffff);
    aligned_write_zeroes(&a, 2);
    aligned_write_zeroes(&a, aligned_remaining_space(&a));
    
    /* crc */
    pp.crc = ~crc_calc32(((byte*)&pp) + sizeof(uint32_t), sizeof(pp) - sizeof(uint32_t));
    
    cps_player_profile_compress_obfuscate_send(client, &pp);
}

void client_send_zone_entry(Client* client)
{
    TlgPacket* packet;
    Aligned a;
    
    packet = packet_create_type(OP_ZoneEntry, PS_ZoneEntry);
    if (!packet) return;
    
    aligned_init(&a, packet_data(packet), packet_length(packet));
    
    /* skip crc for now, it gets written last */
    aligned_advance_by(&a, sizeof(uint32_t));
    /* unknownA */
    aligned_write_zeroes(&a, sizeof_field(PS_ZoneEntry, unknownA));
    /* name */
    aligned_write_snprintf_and_advance_by(&a, sizeof_field(PS_ZoneEntry, name), "%s", client_name_str(client));
    /* zoneShortName */
    aligned_write_snprintf_and_advance_by(&a, sizeof_field(PS_ZoneEntry, zoneShortName), "%s", zone_short_name(client_get_zone(client)));
    /* unknownB */
    aligned_write_zeroes(&a, sizeof_field(PS_ZoneEntry, unknownB));
    /* y */
    aligned_write_float(&a, client_loc_y(client));
    /* x */
    aligned_write_float(&a, client_loc_x(client));
    /* z */
    aligned_write_float(&a, client_loc_z(client));
    /* heading */
    aligned_write_float(&a, client_loc_heading(client) * 2.0); /*fixme: ?*/
    /* unknownC */
    aligned_write_zeroes(&a, sizeof_field(PS_ZoneEntry, unknownC));
    /* guildId */
    aligned_write_uint16(&a, client_guild_id_or_ffff(client));
    /* unknownD */
    aligned_write_zeroes(&a, sizeof_field(PS_ZoneEntry, unknownD));
    /* classId */
    aligned_write_uint8(&a, client_class_id(client));
    /* raceId */
    aligned_write_uint16(&a, client_base_race_id(client));
    /* genderId */
    aligned_write_uint8(&a, client_base_gender_id(client));
    /* level */
    aligned_write_uint8(&a, client_level(client));
    /* unknownE */
    aligned_write_zeroes(&a, sizeof_field(PS_ZoneEntry, unknownE));
    /* isPvP */
    aligned_write_uint8(&a, client_is_pvp(client));
    /* unknownF */
    aligned_write_zeroes(&a, sizeof_field(PS_ZoneEntry, unknownF));
    /* faceId */
    aligned_write_uint8(&a, client_face_id(client));
    /* helmTextureId */
    aligned_write_uint8(&a, client_helm_texture_id(client));
    /* unknownG */
    aligned_write_zeroes(&a, sizeof_field(PS_ZoneEntry, unknownG));
    /* secondaryWeaponModelId */
    aligned_write_uint8(&a, client_secondary_weapon_model_id(client));
    /* primaryWeaponModelId */
    aligned_write_uint8(&a, client_primary_weapon_model_id(client));
    /* unknownH */
    aligned_write_zeroes(&a, sizeof_field(PS_ZoneEntry, unknownH));
    /* helmColor */
    aligned_write_uint32(&a, 0); /*fixme: handle this?*/
    /* unknownI */
    aligned_write_zeroes(&a, sizeof_field(PS_ZoneEntry, unknownI));
    /* textureId */
    aligned_write_uint8(&a, client_texture_id(client));
    /* unknownJ */
    aligned_write_zeroes(&a, sizeof_field(PS_ZoneEntry, unknownJ));
    /* walkingSpeed */
    aligned_write_float(&a, client_base_walking_speed(client));
    /* runningSpeed */
    aligned_write_float(&a, client_base_running_speed(client));
    /* unknownK */
    aligned_write_zeroes(&a, sizeof_field(PS_ZoneEntry, unknownK));
    /* anon */
    aligned_write_uint8(&a, client_anon_setting(client));
    /* unknownL */
    aligned_write_zeroes(&a, sizeof_field(PS_ZoneEntry, unknownL));
    /* surname */
    aligned_write_snprintf_and_advance_by(&a, sizeof_field(PS_ZoneEntry, surname), "%s", client_surname_str_no_null(client));
    /* unknownM */
    aligned_write_zeroes(&a, sizeof_field(PS_ZoneEntry, unknownM));
    /* deityId */
    aligned_write_uint16(&a, client_deity_id(client));
    /* unknownN */
    aligned_write_zeroes(&a, sizeof_field(PS_ZoneEntry, unknownN));
    
    /* crc */
    aligned_reset_cursor(&a);
    aligned_write_uint32(&a, ~crc_calc32(aligned_current(&a) + sizeof(uint32_t), aligned_size(&a) - sizeof(uint32_t)));
    
    client_schedule_packet(client, packet);
}

void client_send_weather(Client* client)
{
    Zone* zone = client_get_zone(client);
    TlgPacket* packet = packet_create_weather(zone_weather_type(zone), zone_weather_intensity(zone));
    
    if (packet)
        client_schedule_packet(client, packet);
}
