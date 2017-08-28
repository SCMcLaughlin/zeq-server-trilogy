
#ifndef PACKET_CREATE_H
#define PACKET_CREATE_H

#include "define.h"
#include "client.h"
#include "inventory.h"
#include "item.h"
#include "item_proto.h"
#include "mob.h"
#include "tlg_packet.h"
#include "zone.h"

TlgPacket* packet_create_zero_filled(uint16_t opcode, uint32_t size);
ZEQ_API TlgPacket* packet_create_weather(int type, int intensity);
TlgPacket* packet_create_zone_info(Zone* zone);
ZEQ_API TlgPacket* packet_create_spawn_appearance(int16_t entityId, uint16_t typeId, int value);
TlgPacket* packet_create_spawn(Mob* mob);
TlgPacket* packet_create_spawns_compressed(Zone* zone);
TlgPacket* packet_create_unspawn(int16_t entityId);
TlgPacket* packet_create_inv_item(Item* item, ItemProto* proto, uint16_t slotId, uint16_t itemId);
ZEQ_API TlgPacket* packet_create_custom_message(uint32_t chatChannel, const char* str, uint32_t len);
TlgPacket* packet_create_custom_message_format(uint32_t chatChannel, const char* fmt, ...);
ZEQ_API TlgPacket* packet_create_spell_cast_begin(Mob* mob, uint32_t spellId, uint32_t castTimeMs);
TlgPacket* packet_create_hp_update_percentage(uint32_t entityId, int8_t hpRatio);
TlgPacket* packet_create_hp_update_self(Client* client);
TlgPacket* packet_create_mana_update(uint16_t mana, uint16_t lastSpellId);
TlgPacket* packet_create_animation(uint32_t entityId, uint32_t animId);
TlgPacket* packet_create_level_update(uint8_t level);
TlgPacket* packet_create_exp_update(uint32_t currentExp);

ZEQ_API TlgPacket* packet_create_test(int16_t entityId, uint16_t spellId, uint32_t whichByte, uint8_t value);

#endif/*PACKET_CREATE_H*/
