
local ffi = require "ffi"

ffi.cdef[[
void client_schedule_packet(void* client, void* packet);
void client_schedule_packet_with_zone(void* client, void* zone, void* packet);

void* packet_create_weather(int type, int intensity);
void* packet_create_spawn_appearance(int16_t entityId, uint32_t typeId, int value);
void* packet_create_custom_message(uint32_t chatChannel, const char* str, uint32_t len);
void* packet_create_spell_cast_begin(void* mob, uint32_t spellId, uint32_t castTimeMs);

void* packet_create_test(int16_t entityId, uint16_t spellId, uint32_t whichByte, uint8_t value);
]]

return ffi.C
