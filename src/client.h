
#ifndef CLIENT_H
#define CLIENT_H

#include "define.h"
#include "buffer.h"
#include "client_load_data.h"
#include "define_netcode.h"
#include "inventory.h"
#include "loc.h"
#include "mob.h"
#include "skills.h"
#include "spellbook.h"
#include "udp_zpacket.h"
#include "zone.h"

typedef struct Client Client;

Client* client_create_unloaded(StaticBuffer* name, int64_t accountId, IpAddr ipAddr, bool isLocal);
Client* client_destroy_no_zone(Client* client);
Client* client_destroy(Client* client);

void client_load_character_data(Client* client, ClientLoadData_Character* data);

void client_calc_stats_all(Client* client);
int64_t client_calc_base_hp(uint8_t classId, int level, int sta);
int64_t client_calc_base_mana(uint8_t classId, int level, int INT, int WIS);

void client_on_unhandled_packet(Client* client, ToServerPacket* packet);
void client_on_msg_command(Client* client, const char* msg, int len);
void client_on_target_by_entity_id(Client* client, int16_t entityId);

ZEQ_API Mob* client_mob(Client* client);
StaticBuffer* client_name(Client* client);
const char* client_name_str(Client* client);
const char* client_surname_str_no_null(Client* client);
uint8_t client_base_gender_id(Client* client);
uint16_t client_base_race_id(Client* client);
uint8_t client_class_id(Client* client);
uint16_t client_deity_id(Client* client);
uint8_t client_level(Client* client);
ZEQ_API int64_t client_experience(Client* client);
uint16_t client_training_points(Client* client);
int64_t client_cur_mana(Client* client);
uint8_t client_face_id(Client* client);
int64_t client_cur_hp(Client* client);
uint8_t client_base_str(Client* client);
uint8_t client_base_sta(Client* client);
uint8_t client_base_cha(Client* client);
uint8_t client_base_dex(Client* client);
uint8_t client_base_int(Client* client);
uint8_t client_base_agi(Client* client);
uint8_t client_base_wis(Client* client);
float client_loc_x(Client* client);
float client_loc_y(Client* client);
float client_loc_z(Client* client);
float client_loc_heading(Client* client);
Coin* client_coin(Client* client);
Coin* client_coin_bank(Client* client);
Coin* client_coin_cursor(Client* client);
uint32_t client_guild_id(Client* client);
uint32_t client_guild_id_or_ffff(Client* client);
uint8_t client_guild_rank(Client* client);
uint8_t client_guild_rank_or_ff(Client* client);
uint8_t client_anon_setting(Client* client);
int client_hp_from_items(Client* client);

Inventory* client_inv(Client* client);
Skills* client_skills(Client* client);
Spellbook* client_spellbook(Client* client);

Zone* client_get_zone(Client* client);
void client_reset_for_zone(Client* client, Zone* zone);
bool client_is_local(Client* client);
IpAddr client_ip_addr(Client* client);
void client_set_ip_addr(Client* client, IpAddr ipAddr);
uint32_t client_ip(Client* client);
uint16_t client_port(Client* client);
void client_set_port(Client* client, uint16_t port);

bool client_is_auto_split_enabled(Client* client);
bool client_is_pvp(Client* client);
bool client_is_gm(Client* client);
bool client_is_afk(Client* client);
bool client_is_linkdead(Client* client);
bool client_is_sneaking(Client* client);
bool client_is_hiding(Client* client);
bool client_has_gm_speed(Client* client);
bool client_has_gm_hide(Client* client);

uint64_t client_disc_timestamp(Client* client);
uint64_t client_harmtouch_timestamp(Client* client);
uint64_t client_creation_timestamp(Client* client);

uint16_t client_hunger(Client* client);
uint16_t client_thirst(Client* client);
uint16_t client_drunkeness(Client* client);

uint8_t client_helm_texture_id(Client* client);
uint8_t client_texture_id(Client* client);
uint8_t client_primary_weapon_model_id(Client* client);
uint8_t client_secondary_weapon_model_id(Client* client);
uint8_t client_upright_state(Client* client);
uint8_t client_light_level(Client* client);
uint8_t client_body_type(Client* client);

float client_base_walking_speed(Client* client);
float client_base_running_speed(Client* client);
float client_walking_speed(Client* client);
float client_running_speed(Client* client);

BindPoint* client_bind_point(Client* client, int n);

int16_t client_entity_id(Client* client);
void client_set_zone_index(Client* client, int index);
int client_zone_index(Client* client);
void client_set_lua_index(Client* client, int index);
int client_lua_index(Client* client);

int client_get_skill(Client* client, int skillId);
/* Returns the previous zone-in count */
uint32_t client_increment_zone_in_count(Client* client);

void client_update_hp(Client* client, int curHp);
#define client_update_hp_with_current(client) client_update_hp((client), (int)mob_cur_hp(client_mob((client))))
void client_update_mana(Client* client, uint16_t curMana);
#define client_update_mana_with_current(client) client_update_mana((client), (uint16_t)mob_cur_mana(client_mob((client))))
ZEQ_API void client_update_level(Client* client, uint8_t level);
ZEQ_API void client_update_exp(Client* client, uint32_t exp);

#endif/*CLIENT_H*/
