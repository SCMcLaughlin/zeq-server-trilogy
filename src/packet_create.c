
#include "packet_create.h"
#include "aligned.h"
#include "packet_struct.h"
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
