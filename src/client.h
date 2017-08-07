
#ifndef CLIENT_H
#define CLIENT_H

#include "define.h"
#include "buffer.h"
#include "client_load_data.h"
#include "define_netcode.h"
#include "inventory.h"
#include "skills.h"
#include "spellbook.h"
#include "zone.h"

typedef struct Client Client;

Client* client_create_unloaded(StaticBuffer* name, int64_t accountId, IpAddr ipAddr, bool isLocal);
Client* client_destroy(Client* client);

void client_load_character_data(Client* client, ClientLoadData_Character* data);

StaticBuffer* client_name(Client* client);
const char* client_surname_str_no_null(Client* client);
uint8_t client_base_gender_id(Client* client);
uint16_t client_base_race_id(Client* client);
uint8_t client_class_id(Client* client);
uint8_t client_level(Client* client);
int64_t client_experience(Client* client);
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

Inventory* client_inv(Client* client);
Skills* client_skills(Client* client);
Spellbook* client_spellbook(Client* client);

Zone* client_get_zone(Client* client);
void client_set_zone(Client* client, Zone* zone);
bool client_is_local(Client* client);
IpAddr client_ip_addr(Client* client);
uint32_t client_ip(Client* client);
uint16_t client_port(Client* client);
void client_set_port(Client* client, uint16_t port);

#endif/*CLIENT_H*/
