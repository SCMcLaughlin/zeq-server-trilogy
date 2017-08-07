
#include "packet_create.h"
#include "aligned.h"
#include "client.h"
#include "mob.h"
#include "packet_struct.h"
#include "tlg_packet.h"
#include "zone.h"
#include "enum_opcode.h"

TlgPacket* packet_create_zero_filled(uint16_t opcode, uint32_t size)
{
    TlgPacket* packet = packet_create(opcode, size);
    if (!packet) return NULL;
    
    memset(packet_data(packet), 0, size);
    return packet;
}

static TlgPacket* packet_init(uint16_t opcode, uint32_t size, Aligned* a)
{
    TlgPacket* packet = packet_create(opcode, size);
    if (!packet) return NULL;
    
    aligned_init(a, packet_data(packet), packet_length(packet));
    return packet;
}

#define packet_init_type(op, type, a) packet_init((op), sizeof(type), (a))

TlgPacket* packet_create_weather(int type, int intensity)
{
    Aligned a;
    TlgPacket* packet = packet_init_type(OP_Weather, PS_Weather, &a);
    if (!packet) return NULL;
    
    /* type */
    aligned_write_int(&a, type);
    /* intensity */
    aligned_write_int(&a, intensity);
    
    return packet;
}

TlgPacket* packet_create_zone_info(Zone* zone)
{
    ZoneFog* fog;
    LocH* safeSpot;
    Aligned a;
    TlgPacket* packet = packet_init_type(OP_ZoneInfo, PS_ZoneInfo, &a);
    if (!packet) return NULL;
    
    /* characterName */
    aligned_write_zeroes(&a, sizeof_field(PS_ZoneInfo, characterName)); /* Unused */
    /* zoneShortName */
    aligned_write_snprintf_and_advance_by(&a, sizeof_field(PS_ZoneInfo, zoneShortName), "%s", zone_short_name(zone));
    /* zoneLongName */
    aligned_write_snprintf_and_advance_by(&a, sizeof_field(PS_ZoneInfo, zoneLongName), "%s", zone_long_name(zone));
    /* zoneType */
    aligned_write_uint8(&a, zone_type(zone));
    
    fog = zone_fog(zone, 0);
    
    /* fogRed[4] */
    aligned_write_uint8(&a, fog[0].red);
    aligned_write_uint8(&a, fog[1].red);
    aligned_write_uint8(&a, fog[2].red);
    aligned_write_uint8(&a, fog[3].red);
    /* fogGreen[4] */
    aligned_write_uint8(&a, fog[0].green);
    aligned_write_uint8(&a, fog[1].green);
    aligned_write_uint8(&a, fog[2].green);
    aligned_write_uint8(&a, fog[3].green);
    /* fogBlue[4] */
    aligned_write_uint8(&a, fog[0].blue);
    aligned_write_uint8(&a, fog[1].blue);
    aligned_write_uint8(&a, fog[2].blue);
    aligned_write_uint8(&a, fog[3].blue);
    
    /* unknownA */
    aligned_write_zeroes(&a, sizeof_field(PS_ZoneInfo, unknownA));
    
    /* fogMinClippingDistance[4] */
    aligned_write_float(&a, fog[0].minClippingDistance);
    aligned_write_float(&a, fog[1].minClippingDistance);
    aligned_write_float(&a, fog[2].minClippingDistance);
    aligned_write_float(&a, fog[3].minClippingDistance);
    /* fogMaxClippingDistance[4] */
    aligned_write_float(&a, fog[0].maxClippingDistance);
    aligned_write_float(&a, fog[1].maxClippingDistance);
    aligned_write_float(&a, fog[2].maxClippingDistance);
    aligned_write_float(&a, fog[3].maxClippingDistance);
    
    /* gravity */
    aligned_write_float(&a, zone_gravity(zone));
    
    /* unknownB[50] */
    aligned_write_uint8(&a, 0x20);
    aligned_write_memset(&a, 0x0a, 4);
    aligned_write_uint8(&a, 0x18);
    aligned_write_uint8(&a, 0x06);
    aligned_write_uint8(&a, 0x02);
    aligned_write_uint8(&a, 0x0a);
    aligned_write_zeroes(&a, 8);
    aligned_write_memset(&a, 0xff, 32);
    aligned_write_uint8(&a, 0);
    
    /* skyType */
    aligned_write_uint16(&a, zone_sky_type(zone));
    
    /* unknownC[8] */
    aligned_write_uint8(&a, 0x04);
    aligned_write_uint8(&a, 0);
    aligned_write_uint8(&a, 0x02);
    aligned_write_zeroes(&a, 5);
    
    /* unknownD */
    aligned_write_float(&a, 0.75f);
    
    safeSpot = zone_safe_spot(zone);
    
    /* safeSpotX */
    aligned_write_float(&a, safeSpot->x);
    /* safeSpotY */
    aligned_write_float(&a, safeSpot->y);
    /* safeSpotZ */
    aligned_write_float(&a, safeSpot->z);
    /* safeSpotHeading */
    aligned_write_float(&a, safeSpot->heading);
    /* minZ */
    aligned_write_float(&a, zone_min_z(zone));
    /* minClippingDistance */
    aligned_write_float(&a, zone_min_clip_dist(zone));
    /* maxClippingDistance */
    aligned_write_float(&a, zone_max_clip_dist(zone));
    /* unknownE */
    aligned_write_zeroes(&a, sizeof_field(PS_ZoneInfo, unknownE));
    
    return packet;
}

TlgPacket* packet_create_spawn_appearance(int16_t entityId, int16_t typeId, int value)
{
    Aligned a;
    TlgPacket* packet = packet_init_type(OP_SpawnAppearance, PS_SpawnAppearance, &a);
    if (!packet) return NULL;
    
    /* entityId */
    aligned_write_int16(&a, entityId);
    /* unknownA */
    aligned_write_uint16(&a, 0);
    /* typeId */
    aligned_write_int16(&a, typeId);
    /* unknownB */
    aligned_write_uint16(&a, 0);
    /* value */
    aligned_write_int(&a, value);
    
    return packet;
}

static void packet_obfuscate_spawn(byte* data, uint32_t len)
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

TlgPacket* packet_create_spawn(Mob* mob)
{
    Aligned a;
    int mobType;
    int temp;
    TlgPacket* packet = packet_init_type(OP_Spawn, PS_Spawn, &a);
    
    if (!packet) return NULL;
    
    mobType = mob_parent_type(mob);
    
    /* unknownA */
    aligned_write_uint32(&a, 0);
    /* size */
    aligned_write_float(&a, mob_cur_size(mob));
    /* walkingSpeed */
    aligned_write_float(&a, mob_cur_walking_speed(mob));
    /* runningSpeed */
    aligned_write_float(&a, mob_cur_running_speed(mob));
    
    /* tints[7] */
    aligned_write_uint32(&a, mob_tint(mob, 0));
    aligned_write_uint32(&a, mob_tint(mob, 1));
    aligned_write_uint32(&a, mob_tint(mob, 2));
    aligned_write_uint32(&a, mob_tint(mob, 3));
    aligned_write_uint32(&a, mob_tint(mob, 4));
    aligned_write_uint32(&a, mob_tint(mob, 5));
    aligned_write_uint32(&a, mob_tint(mob, 6));
    
    /* unknownB */
    aligned_write_zeroes(&a, sizeof_field(PS_Spawn, unknownB));
    /* heading */
    aligned_write_int8(&a, (int8_t)mob->headingRaw); /*fixme: decide how to deal with headings*/
    /* headingDelta */
    aligned_write_int8(&a, 0); /*fixme: handle deltas*/
    /* y */
    aligned_write_int16(&a, (int16_t)mob_y(mob));
    /* x */
    aligned_write_int16(&a, (int16_t)mob_x(mob));
    /* z */
    aligned_write_int16(&a, (int16_t)(mob_z(mob) * 10)); /* The client expects this for more accurate Z values after decimal truncation */
    
    /* delta bitfield */
    aligned_write_int(&a, 0); /*fixme: deal with this properly...*/
    
    /* unknownC */
    aligned_write_uint8(&a, 0);
    /* entityId */
    aligned_write_int16(&a, mob_entity_id(mob));
    /* bodyType */
    aligned_write_uint16(&a, mob_body_type(mob));
    /* ownerEntityId */
    aligned_write_int16(&a, mob_owner_entity_id(mob));
    /* hpPercent */
    aligned_write_int16(&a, mob_hp_ratio(mob));
    
    /* guildId */
    if (mobType == MOB_PARENT_TYPE_Client)
        aligned_write_uint16(&a, client_guild_id_or_ffff(mob_as_client(mob)));
    else
        aligned_write_uint16(&a, 0xffff);
    
    /* raceId */
    temp = mob_race_id(mob);
    aligned_write_uint8(&a, (temp > 0xff) ? 1 : temp); /* Seems like raceId is 16bit everywhere but here... */
    
    switch (mobType)
    {
    case MOB_PARENT_TYPE_Npc:
    case MOB_PARENT_TYPE_Pet:
        temp = 1;
        break;
    
    case MOB_PARENT_TYPE_Client:
        temp = 0;
        break;
    
    case MOB_PARENT_TYPE_NpcCorpse:
        temp = 3;
        break;
    
    case MOB_PARENT_TYPE_ClientCorpse:
        temp = 2;
        break;
    }
    
    /* mobType */
    aligned_write_uint8(&a, (uint8_t)temp);
    /* classId */
    aligned_write_uint8(&a, mob_class_id(mob));
    /* genderId */
    aligned_write_uint8(&a, mob_gender_id(mob));
    /* level */
    aligned_write_uint8(&a, mob_level(mob));
    /* isInvisible */
    aligned_write_uint8(&a, 0); /*fixme: handle invisibility*/
    /* unknownD */
    aligned_write_uint8(&a, 0);
    
    /* isPvP */
    if (mobType == MOB_PARENT_TYPE_Client)
        aligned_write_uint8(&a, client_is_pvp(mob_as_client(mob)));
    else
        aligned_write_uint8(&a, 0);
    
    /* uprightState */
    aligned_write_uint8(&a, mob_upright_state(mob));
    /* lightLevel */
    aligned_write_uint8(&a, mob_light_level(mob));
    
    if (mobType == MOB_PARENT_TYPE_Client)
    {
        Client* client = mob_as_client(mob);
        
        /* anon */
        aligned_write_uint8(&a, client_anon_setting(client));
        /* isAfk */
        aligned_write_uint8(&a, client_is_afk(client));
        /* unknownE */
        aligned_write_uint8(&a, 0);
        /* isLinkdead */
        aligned_write_uint8(&a, client_is_linkdead(client));
        /* isGM */
        aligned_write_uint8(&a, client_is_gm(client));
    }
    else
    {
        aligned_write_zeroes(&a, 5);
    }
    
    /* unknownF */
    aligned_write_uint8(&a, 0);
    /* textureId */
    aligned_write_uint8(&a, mob_texture_id(mob));
    /* helmTextureId */
    aligned_write_uint8(&a, mob_helm_texture_id(mob));
    /* unknownG */
    aligned_write_uint8(&a, 0);
    
    /* materialIds[9] */
    aligned_write_uint8(&a, mob_material_id(mob, 0));
    aligned_write_uint8(&a, mob_material_id(mob, 1));
    aligned_write_uint8(&a, mob_material_id(mob, 2));
    aligned_write_uint8(&a, mob_material_id(mob, 3));
    aligned_write_uint8(&a, mob_material_id(mob, 4));
    aligned_write_uint8(&a, mob_material_id(mob, 5));
    aligned_write_uint8(&a, mob_material_id(mob, 6));
    aligned_write_uint8(&a, mob_primary_weapon_model_id(mob));
    aligned_write_uint8(&a, mob_secondary_weapon_model_id(mob));
    
    /* name */
    aligned_write_snprintf_and_advance_by(&a, sizeof_field(PS_Spawn, name), "%s", mob_name_str(mob));
    
    if (mobType == MOB_PARENT_TYPE_Client)
    {
        Client* client = mob_as_client(mob);
        
        /* surname */
        aligned_write_snprintf_and_advance_by(&a, sizeof_field(PS_Spawn, surname), "%s", client_surname_str_no_null(client));
        /* guildRank */
        aligned_write_uint8(&a, client_guild_rank_or_ff(client));
    }
    else
    {
        /* surname, guildRank */
        aligned_write_zeroes(&a, sizeof_field(PS_Spawn, surname) + sizeof_field(PS_Spawn, guildRank));
    }
    
    /* unknownH */
    aligned_write_uint8(&a, 0);
    /* deityId */
    aligned_write_uint16(&a, mob_deity_id(mob));
    /* unknownI */
    aligned_write_zeroes(&a, sizeof_field(PS_Spawn, unknownI));
    
    packet_obfuscate_spawn(packet_data(packet), packet_length(packet));
    
    return packet;
}
