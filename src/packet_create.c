
#include "packet_create.h"
#include "aligned.h"
#include "client.h"
#include "mob.h"
#include "packet_convert.h"
#include "packet_struct.h"
#include "tlg_packet.h"
#include "util_alloc.h"
#include "util_cap.h"
#include "zone.h"
#include "enum_opcode.h"
#include <zlib.h>

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
    
    /* PS_Weather */
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
    
    /* PS_ZoneInfo */
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

TlgPacket* packet_create_spawn_appearance(int16_t entityId, uint32_t typeId, int value)
{
    Aligned a;
    TlgPacket* packet = packet_init_type(OP_SpawnAppearance, PS_SpawnAppearance, &a);
    if (!packet) return NULL;
    
    /* PS_SpawnAppearance */
    /* entityId */
    aligned_write_uint32(&a, (uint32_t)entityId);
    /* typeId */
    aligned_write_uint32(&a, typeId);
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

static void packet_obfuscate_spawns_compressed(byte* data, uint32_t len)
{
    uint32_t* ptr = (uint32_t*)data;
    uint32_t swap;
    
    /* Extra hoop to jump through */
    swap = ptr[0];
    ptr[0] = ptr[len / 8];
    ptr[len / 8] = swap;
    
    packet_obfuscate_spawn(data, len);
}

static void packet_fill_spawn(Mob* mob, Aligned* a)
{
    int mobType;
    int temp;
    
    mobType = mob_parent_type(mob);
    
    /* PS_Spawn */
    /* unknownA */
    aligned_write_uint32(a, 0);
    /* size */
    aligned_write_float(a, mob_cur_size(mob));
    /* walkingSpeed */
    aligned_write_float(a, mob_cur_walking_speed(mob));
    /* runningSpeed */
    aligned_write_float(a, mob_cur_running_speed(mob));
    
    /* tints[7] */
    aligned_write_uint32(a, mob_tint(mob, 0));
    aligned_write_uint32(a, mob_tint(mob, 1));
    aligned_write_uint32(a, mob_tint(mob, 2));
    aligned_write_uint32(a, mob_tint(mob, 3));
    aligned_write_uint32(a, mob_tint(mob, 4));
    aligned_write_uint32(a, mob_tint(mob, 5));
    aligned_write_uint32(a, mob_tint(mob, 6));
    
    /* unknownB */
    aligned_write_zeroes(a, sizeof_field(PS_Spawn, unknownB));
    /* heading */
    aligned_write_int8(a, (int8_t)mob->headingRaw); /*fixme: decide how to deal with headings*/
    /* headingDelta */
    aligned_write_int8(a, 0); /*fixme: handle deltas*/
    /* y */
    aligned_write_int16(a, (int16_t)mob_y(mob));
    /* x */
    aligned_write_int16(a, (int16_t)mob_x(mob));
    /* z */
    aligned_write_int16(a, (int16_t)(mob_z(mob) * 10)); /* The client expects this for more accurate Z values after decimal truncation */
    
    /* delta bitfield */
    aligned_write_int(a, 0); /*fixme: deal with this properly...*/
    
    /* unknownC */
    aligned_write_uint8(a, 0);
    /* entityId */
    aligned_write_int16(a, mob_entity_id(mob));
    /* bodyType */
    aligned_write_uint16(a, mob_body_type(mob));
    /* ownerEntityId */
    aligned_write_int16(a, mob_owner_entity_id(mob));
    /* hpPercent */
    aligned_write_int16(a, mob_hp_ratio(mob));
    
    /* guildId */
    if (mobType == MOB_PARENT_TYPE_Client)
        aligned_write_uint16(a, client_guild_id_or_ffff(mob_as_client(mob)));
    else
        aligned_write_uint16(a, 0xffff);
    
    /* raceId */
    temp = mob_race_id(mob);
    aligned_write_uint8(a, (temp > 0xff) ? 1 : temp); /* Seems like raceId is 16bit everywhere but here... */
    
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
    aligned_write_uint8(a, (uint8_t)temp);
    /* classId */
    aligned_write_uint8(a, mob_class_id(mob));
    /* genderId */
    aligned_write_uint8(a, mob_gender_id(mob));
    /* level */
    aligned_write_uint8(a, mob_level(mob));
    /* isInvisible */
    aligned_write_uint8(a, 0); /*fixme: handle invisibility*/
    /* unknownD */
    aligned_write_uint8(a, 0);
    
    /* isPvP */
    if (mobType == MOB_PARENT_TYPE_Client)
        aligned_write_uint8(a, client_is_pvp(mob_as_client(mob)));
    else
        aligned_write_uint8(a, 0);
    
    /* uprightState */
    aligned_write_uint8(a, mob_upright_state(mob));
    /* lightLevel */
    aligned_write_uint8(a, mob_light_level(mob));
    
    if (mobType == MOB_PARENT_TYPE_Client)
    {
        Client* client = mob_as_client(mob);
        
        /* anon */
        aligned_write_uint8(a, client_anon_setting(client));
        /* isAfk */
        aligned_write_uint8(a, client_is_afk(client));
        /* unknownE */
        aligned_write_uint8(a, 0);
        /* isLinkdead */
        aligned_write_uint8(a, client_is_linkdead(client));
        /* isGM */
        aligned_write_uint8(a, client_is_gm(client));
    }
    else
    {
        aligned_write_zeroes(a, 5);
    }
    
    /* unknownF */
    aligned_write_uint8(a, 0);
    /* textureId */
    aligned_write_uint8(a, mob_texture_id(mob));
    /* helmTextureId */
    aligned_write_uint8(a, mob_helm_texture_id(mob));
    /* unknownG */
    aligned_write_uint8(a, 0);
    
    /* materialIds[9] */
    aligned_write_uint8(a, mob_material_id(mob, 0));
    aligned_write_uint8(a, mob_material_id(mob, 1));
    aligned_write_uint8(a, mob_material_id(mob, 2));
    aligned_write_uint8(a, mob_material_id(mob, 3));
    aligned_write_uint8(a, mob_material_id(mob, 4));
    aligned_write_uint8(a, mob_material_id(mob, 5));
    aligned_write_uint8(a, mob_material_id(mob, 6));
    aligned_write_uint8(a, mob_primary_weapon_model_id(mob));
    aligned_write_uint8(a, mob_secondary_weapon_model_id(mob));
    
    /* name */
    aligned_write_snprintf_and_advance_by(a, sizeof_field(PS_Spawn, name), "%s", mob_name_str(mob));
    
    if (mobType == MOB_PARENT_TYPE_Client)
    {
        Client* client = mob_as_client(mob);
        
        /* surname */
        aligned_write_snprintf_and_advance_by(a, sizeof_field(PS_Spawn, surname), "%s", client_surname_str_no_null(client));
        /* guildRank */
        aligned_write_uint8(a, client_guild_rank_or_ff(client));
    }
    else
    {
        /* surname, guildRank */
        aligned_write_zeroes(a, sizeof_field(PS_Spawn, surname) + sizeof_field(PS_Spawn, guildRank));
    }
    
    /* unknownH */
    aligned_write_uint8(a, 0);
    /* deityId */
    aligned_write_uint16(a, mob_deity_id(mob));
    /* unknownI */
    aligned_write_zeroes(a, sizeof_field(PS_Spawn, unknownI));
}

TlgPacket* packet_create_spawn(Mob* mob)
{
    Aligned a;
    TlgPacket* packet = packet_init_type(OP_Spawn, PS_Spawn, &a);
    if (!packet) return NULL;
    
    packet_fill_spawn(mob, &a);
    packet_obfuscate_spawn(packet_data(packet), packet_length(packet));
    
    return packet;
}

TlgPacket* packet_spawns_compress_obfuscate(Zone* zone, byte* data, uint32_t len)
{
    byte* buffer = alloc_bytes(len);
    unsigned long length = len;
    TlgPacket* packet = NULL;
    int rc;
    
    if (!buffer) 
    {
        log_writef(zone_log_queue(zone), zone_log_id(zone), "ERROR: packet_spawns_compress_obfuscate: allocation failed for %u byte buffer", len);
        goto fail_alloc;
    }
    
    rc = compress2(buffer, &length, data, len, Z_BEST_COMPRESSION);
    
    if (rc != Z_OK)
    {
        log_writef(zone_log_queue(zone), zone_log_id(zone), "ERROR: packet_spawns_compress_obfuscate: compress2() failed, errcode: %i", rc);
        goto fail;
    }
    
    packet_obfuscate_spawns_compressed(buffer, (uint32_t)length);
    
    packet = packet_create(OP_SpawnsCompressed, (uint32_t)length);
    
    if (packet)
    {
        memcpy(packet_data(packet), buffer, length);
    }
    else
    {
        log_writef(zone_log_queue(zone), zone_log_id(zone), "ERROR: packet_spawns_compress_obfuscate: packet_create() failed for %u byte compressed spawns buffer", length);
    }
    
fail:
    free(buffer);
fail_alloc:
    free(data);
    return packet;
}

TlgPacket* packet_create_spawns_compressed(Zone* zone)
{
    Mob** mobs = zone_mob_list(zone);
    uint32_t n = zone_mob_count(zone);
    uint32_t len, i;
    byte* data;
    Aligned a;
    
    if (n == 0) return NULL;
    
    len = n * sizeof(PS_Spawn);
    data = alloc_bytes(len);
    
    if (!data)
    {
        log_writef(zone_log_queue(zone), zone_log_id(zone), "ERROR: packet_create_spawns_compressed: allocation failed for %u byte buffer to contain %u spawns", len, n);
        return NULL;
    }
    
    aligned_init(&a, data, len);
    
    for (i = 0; i < n; i++)
    {
        /* No real good way of doing this, have to dereference every Mob */
        packet_fill_spawn(mobs[i], &a);
    }
    
    return packet_spawns_compress_obfuscate(zone, data, len);
}

TlgPacket* packet_create_unspawn(int16_t entityId)
{
    Aligned a;
    TlgPacket* packet = packet_init_type(OP_Inventory, PS_Item, &a);
    
    if (!packet) return NULL;

    /* PS_Unspawn */
    /* entityId */
    aligned_write_int16(&a, entityId);
    /* unknown */
    aligned_write_uint32(&a, 0);

    return packet;
}

TlgPacket* packet_create_inv_item(Item* item, ItemProto* proto, uint16_t slotId, uint16_t itemId)
{
    Aligned a;
    PC_Item citem;
    uint16_t temp = 0;
    TlgPacket* packet = packet_init_type(OP_Inventory, PS_Item, &a);
    
    if (!packet) return NULL;
    
    pc_item_set_defaults(&citem);
    
    citem.itemId = itemId;
    citem.currentSlot = slotId;
    
    item_proto_to_packet(proto, &citem);
    
    /*fixme: handle charges, stack amount*/
    
    /* PS_Item */
    /* name */
    aligned_write_snprintf_and_advance_by(&a, sizeof_field(PS_Item, name), "%s", sbuf_str_or_empty_string(citem.name));
    
    /* tagFlag */
    /* The first character of the lore text field controls the LORE ITEM, NO RENT, ARTIFACT and PENDING LORE flags, if present */
    if (citem.isUnique)
        temp = '*';
    else if (!citem.isPermanent)
        temp = '&';
    
    if (temp)
    {
        aligned_write_uint8(&a, (uint8_t)temp);
        temp = 1;
    }
    
    /* lore */
    aligned_write_snprintf_and_advance_by(&a, sizeof_field(PS_Item, lore) - temp, "%s", sbuf_str_or_empty_string(citem.lore));
    /* model */
    aligned_write_snprintf_and_advance_by(&a, sizeof_field(PS_Item, model), "IT%u", citem.model);
    
    /* typeFlag */
    if (citem.isBag)
        temp = 0x5400;
    else if (citem.isBook)
        temp = 0x7669;
    else if (citem.isScroll)
        temp = 0;
    else
        temp = 0x3336;
    
    aligned_write_uint16(&a, temp);
    
    /* unknownA */
    aligned_write_zeroes(&a, sizeof_field(PS_Item, unknownA));
    /* weight */
    aligned_write_uint8(&a, citem.weight);
    /* isPermanent */
    aligned_write_uint8(&a, (citem.isPermanent) ? 255 : 0);
    /* isDroppable */
    aligned_write_uint8(&a, (citem.isDroppable) ? 1 : 0);
    /* size */
    aligned_write_uint8(&a, citem.size);
    
    /* itemType */
    if (citem.isBag)
        temp = 1;
    else if (citem.isBook || citem.isScroll)
        temp = 2;
    else
        temp = 0;
    
    aligned_write_uint8(&a, (uint8_t)temp);
    
    /* itemId */
    aligned_write_uint16(&a, citem.itemId);
    /* icon */
    aligned_write_uint16(&a, citem.icon);
    /* currentSlot */
    aligned_write_uint16(&a, citem.currentSlot);
    /* slotsBitfield */
    aligned_write_uint32(&a, citem.slots);
    /* cost */
    aligned_write_uint32(&a, citem.cost);
    /* unknownB, instanceId */
    aligned_write_zeroes(&a, sizeof_field(PS_Item, unknownB) + sizeof_field(PS_Item, instanceId));
    /* isDroppableRoleplayServer */
    aligned_write_uint8(&a, (citem.isDroppable) ? 1 : 0);
    /* unknownC */
    aligned_write_zeroes(&a, sizeof_field(PS_Item, unknownC));
    
    if (citem.isBook)
    {
        /* PS_ItemBook */
        /* isBook */
        aligned_write_uint8(&a, 1);
        /* type */
        aligned_write_uint16(&a, 0); /*fixme*/
        /* fileName */
        aligned_write_snprintf_and_advance_by(&a, sizeof_field(PS_ItemBook, fileName), "%u", item_proto_item_id(proto));
        /* unknownA */
        aligned_write_zeroes(&a, sizeof_field(PS_ItemBook, unknownA));
    }
    else
    {
        /* PS_ItemStats */
        /* STR */
        aligned_write_int8(&a, citem.STR);
        /* STA */
        aligned_write_int8(&a, citem.STA);
        /* CHA */
        aligned_write_int8(&a, citem.CHA);
        /* DEX */
        aligned_write_int8(&a, citem.DEX);
        /* INT */
        aligned_write_int8(&a, citem.INT);
        /* AGI */
        aligned_write_int8(&a, citem.AGI);
        /* WIS */
        aligned_write_int8(&a, citem.WIS);
        /* MR */
        aligned_write_int8(&a, citem.MR);
        /* FR */
        aligned_write_int8(&a, citem.FR);
        /* CR */
        aligned_write_int8(&a, citem.CR);
        /* DR */
        aligned_write_int8(&a, citem.DR);
        /* PR */
        aligned_write_int8(&a, citem.PR);
        /* hp */
        aligned_write_int8(&a, citem.hp);
        /* mana */
        aligned_write_int8(&a, citem.mana);
        /* AC */
        aligned_write_int8(&a, citem.AC);
        /* isStackable/hasUnlimitedCharges */
        aligned_write_uint8(&a, (citem.isStackable || citem.isUnlimitedCharges) ? 1 : 0);
        /* isTestItem */
        aligned_write_uint8(&a, 0);
        /* light */
        aligned_write_uint8(&a, citem.light);
        /* delay */
        aligned_write_uint8(&a, citem.delay);
        /* damage */
        aligned_write_uint8(&a, citem.damage);
        /* clickyType */
        aligned_write_uint8(&a, citem.clickyType);
        /* range */
        aligned_write_uint8(&a, citem.range);
        /* skill */
        aligned_write_uint8(&a, citem.itemType);
        /* isMagic */
        aligned_write_uint8(&a, (citem.isMagic) ? 1 : 0);
        /* clickableLevel */
        aligned_write_uint8(&a, citem.clickableLevel);
        /* materialId */
        aligned_write_uint8(&a, citem.materialId);
        /* unknownA */
        aligned_write_uint16(&a, 0);
        /* tint */
        aligned_write_uint32(&a, citem.tint);
        /* unknownB */
        aligned_write_uint16(&a, 0);
        /* spellId */
        aligned_write_uint16(&a, citem.spellId);
        /* classesBitfield */
        aligned_write_uint32(&a, citem.classes);
        
        if (citem.isBag)
        {
            /* type */
            aligned_write_uint8(&a, citem.bag.type); /*fixme: tradeskill types etc*/
            /* capacity */
            aligned_write_uint8(&a, citem.bag.capacity);
            /* isOpen */
            aligned_write_uint8(&a, 0);
            /* containableSize */
            aligned_write_uint8(&a, citem.bag.containableSize);
            /* weightReductionPercent */
            aligned_write_uint8(&a, citem.bag.weightReductionPercent);
        }
        else
        {
            /* racesBitfield */
            aligned_write_uint32(&a, citem.races);
            /* consumableType */
            aligned_write_uint8(&a, 3); /*fixme: 3 for clickies (and procs?), should any other values go here? */
        }
        
        /* procLevel/hastePercent */
        /*fixme: procLevel is meaningless to the client, and doesn't seem like this affects button refresh times...*/
        aligned_write_uint8(&a, citem.hastePercent);
        
        /* charges */
        aligned_write_uint8(&a, item->charges);
        /* effectType */
        aligned_write_uint8(&a, citem.effectType);
        /* clickySpellId */
        aligned_write_uint16(&a, citem.clickySpellId);
        /* unknownC */
        aligned_write_zeroes(&a, sizeof_field(PS_ItemStats, unknownC));
        /* castingTime */
        aligned_write_uint32(&a, citem.castingTime);
        /* unknownD, recommendedLevel, unknownE */
        aligned_write_zeroes(&a, sizeof_field(PS_ItemStats, unknownD) + sizeof_field(PS_ItemStats, recommendedLevel) + sizeof_field(PS_ItemStats, unknownE));
        /* deityBitfield */
        aligned_write_uint32(&a, 0); /*fixme*/
    }

    return packet;
}

TlgPacket* packet_create_custom_message(uint32_t chatChannel, const char* str, uint32_t len)
{
    Aligned a;
    TlgPacket* packet;
    
    len++; /* Include null terminator */
    
    packet = packet_init(OP_CustomMessage, sizeof(PS_CustomMessage) + len, &a);
    if (!packet) return NULL;
    
    /* PS_CustomMessage */
    /* chatChannel */
    aligned_write_uint32(&a, chatChannel);
    /* message */
    aligned_write_buffer(&a, str, len);
    
    return packet;
}

TlgPacket* packet_create_custom_message_format(uint32_t chatChannel, const char* fmt, ...)
{
    char buf[4096];
    int len;
    va_list args;
    
    va_start(args, fmt);
    len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    
    if (len <= 0) return NULL;
    
    if (len >= (int)sizeof(buf))
        len = (int)(sizeof(buf) - 1);
    
    return packet_create_custom_message(chatChannel, buf, (uint32_t)len);
}

TlgPacket* packet_create_spell_cast_begin(Mob* mob, uint32_t spellId, uint32_t castTimeMs)
{
    Aligned a;
    TlgPacket* packet = packet_init_type(OP_SpellCastBegin, PS_SpellCastBegin, &a);
    if (!packet) return NULL;
    
    /* PS_SpellCastBegin */
    /* casterId */
    aligned_write_uint32(&a, (uint32_t)mob_entity_id(mob));
    /* spellId */
    aligned_write_uint32(&a, spellId);
    /* castTimeMs */
    aligned_write_uint32(&a, castTimeMs);
    
    return packet;
}

TlgPacket* packet_create_hp_update_percentage(uint32_t entityId, int8_t hpRatio)
{
    Aligned a;
    TlgPacket* packet = packet_init_type(OP_HpUpdate, PS_HpUpdate, &a);
    if (!packet) return NULL;
    
    /* PS_HpUpdate */
    /* entityId */
    aligned_write_uint32(&a, entityId);
    /* curHp */
    aligned_write_int(&a, (int)hpRatio);
    /* maxHp */
    aligned_write_int(&a, 100);
    
    return packet;
}

TlgPacket* packet_create_hp_update_self(Client* client)
{
    Mob* mob;
    int curHp, maxHp, hpFromItems;
    Aligned a;
    TlgPacket* packet = packet_init_type(OP_HpUpdate, PS_HpUpdate, &a);
    if (!packet) return NULL;
    
    hpFromItems = client_hp_from_items(client);
    mob = client_mob(client);
    curHp = cap_max((int)(mob_cur_hp(mob) - hpFromItems), 30000);
    maxHp = cap_max((int)(mob_max_hp(mob) - hpFromItems), 30000);
    
    /* PS_HpUpdate */
    /* entityId */
    aligned_write_uint32(&a, (uint32_t)mob_entity_id(mob));
    /* curHp */
    aligned_write_int(&a, curHp);
    /* maxHp */
    aligned_write_int(&a, maxHp);
    
    return packet;
}

TlgPacket* packet_create_mana_update(uint16_t mana, uint16_t lastSpellId)
{
    Aligned a;
    TlgPacket* packet = packet_init_type(OP_ManaUpdate, PS_ManaUpdate, &a);
    if (!packet) return NULL;
    
    /* PS_ManaUpdate */
    /* mana */
    aligned_write_uint16(&a, mana);
    /* lastSpellId */
    aligned_write_uint16(&a, lastSpellId);
    
    return packet;
}

TlgPacket* packet_create_animation(uint32_t entityId, uint32_t animId)
{
    Aligned a;
    TlgPacket* packet = packet_init_type(OP_Animation, PS_Animation, &a);
    if (!packet) return NULL;
    
    /* PS_Animation */
    /* entityId */
    aligned_write_uint32(&a, entityId);
    /* animId */
    aligned_write_uint32(&a, animId);
    /* unknown */
    aligned_write_uint32(&a, 0x00003f80);
    
    return packet;
}

TlgPacket* packet_create_level_update(uint8_t level)
{
    Aligned a;
    TlgPacket* packet = packet_init_type(OP_LevelUpdate, PS_LevelUpdate, &a);
    if (!packet) return NULL;

    /* PS_LevelUpdate */
    /* level */
    aligned_write_uint8(&a, level);
    /* flagAlwaysOne */
    aligned_write_uint8(&a, 1);

    return packet;
}

TlgPacket* packet_create_exp_update(uint32_t currentExp)
{
    Aligned a;
    TlgPacket* packet = packet_init_type(OP_ExpUpdate, PS_ExpUpdate, &a);
    if (!packet) return NULL;

    /* PS_ExpUpdate */
    /* currentExp */
    aligned_write_uint32(&a, currentExp);

    return packet;
}

TlgPacket* packet_create_test(int16_t entityId, uint16_t spellId, uint32_t whichByte, uint8_t value)
{
    Aligned a;
    TlgPacket* packet = packet_init_type(OP_SpellCastFinish, PS_SpellCastFinish, &a);
    byte* ptr;
    if (!packet) return NULL;

    /* PS_SpellCastFinish */

    aligned_write_uint32(&a, (uint32_t)entityId);
    aligned_write_uint32(&a, (uint32_t)entityId);
    aligned_write_uint8(&a, 1);
    aligned_write_uint8(&a, 0);
    aligned_write_uint8(&a, 65);
    aligned_write_uint8(&a, 0);
    aligned_write_uint8(&a, 10);
    aligned_write_memset(&a, 0, 7);
    aligned_write_float(&a, 0.0f);
    aligned_write_uint32(&a, 0);
    aligned_write_uint32(&a, 231);
    aligned_write_uint16(&a, spellId);
    aligned_write_uint8(&a, 0);
    aligned_write_uint8(&a, 4);

    ptr = aligned_all(&a);
    if (whichByte < aligned_size(&a))
        ptr[whichByte] = value;

    return packet;
}
