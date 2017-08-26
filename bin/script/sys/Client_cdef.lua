
local ffi = require "ffi"

ffi.cdef[[
void* client_mob(void* client);
int64_t client_experience(void* client);

void client_update_hp(void* client, int curHp);
void client_update_mana(void* client, uint16_t curMana);
void client_update_level(void* client, uint8_t level);
void client_update_exp(void* client, uint32_t exp);
]]

return ffi.C
