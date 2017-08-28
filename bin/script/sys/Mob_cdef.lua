
local ffi = require "ffi"

ffi.cdef[[
const char* mob_name_str(void* mob);
int64_t mob_cur_hp(void* mob);
int64_t mob_max_hp(void* mob);
int64_t mob_cur_mana(void* mob);
int64_t mob_max_mana(void* mob);
uint8_t mob_level(void* mob);
uint8_t mob_class_id(void* mob);
uint8_t mob_gender_id(void* mob);
void mob_update_gender_id(void* mob, uint8_t genderId);
uint16_t mob_race_id(void* mob);
void mob_update_race_id(void* mob, uint16_t raceId);
float mob_x(void* mob);
float mob_y(void* mob);
float mob_z(void* mob);
void* mob_get_zone(void* mob);
int16_t mob_entity_id(void* mob);
int mob_lua_index(void* mob);
float mob_cur_size(void* mob);
uint8_t mob_texture_id(void* mob);
void mob_update_texture_id(void* mob, uint8_t textureId);
uint8_t mob_helm_texture_id(void* mob);
void mob_update_helm_texture_id(void* mob, uint8_t textureId);
void* mob_target(void* mob);
void mob_update_level(void* mob, uint8_t level);
void mob_update_size(void* mob, float size);
void mob_animate_nearby(void* mob, uint32_t animId);
void mob_animate_range(void* mob, uint32_t animId, double range);
]]

return ffi.C
