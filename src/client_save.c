
#include "client_save.h"
#include "client.h"
#include "db_thread.h"
#include "enum_zop.h"
#include "inventory.h"
#include "spellbook.h"
#include "util_alloc.h"
#include "zone.h"
#include "zpacket.h"

void client_save(Client* client)
{
    ClientSave* save = alloc_type(ClientSave);
    Mob* mob;
    Spellbook* spells;
    MemmedSpell* memmed;
    Zone* zone;
    Inventory* inv;
    ZPacket query;
    uint32_t i;
    int rc;

    if (!save) return;

    save->items = NULL;
    save->cursorQueue = NULL;
    save->spellbook = NULL;
    
    mob = client_mob(client);
    spells = client_spellbook(client);
    memmed = spellbook_memmed_spells(spells);
    zone = client_get_zone(client);
    inv = client_inv(client);

    save->characterId = client_character_id(client);
    save->experience = client_experience(client);
    save->harmtouchTimestamp = client_harmtouch_timestamp(client);
    save->disciplineTimestamp = client_disc_timestamp(client);
    save->creationTimestamp = client_creation_timestamp(client);
    save->curHp = mob_cur_hp(mob);
    save->curMana = mob_cur_mana(mob);
    save->name = client_name(client);
    save->surname = client_surname(client);
    save->ip = client_ip(client);
    save->level = mob_level(mob);
    save->classId = client_class_id(client);
    save->genderId = client_base_gender_id(client);
    save->faceId = client_face_id(client);
    save->guildRank = client_guild_rank(client);
    save->isGM = client_is_gm(client);
    save->isAutosplit = client_is_auto_split_enabled(client);
    save->isPvP = client_is_pvp(client);
    save->anonSetting = client_anon_setting(client);
    save->raceId = client_base_race_id(client);
    save->deityId = client_deity_id(client);
    save->trainingPoints = client_training_points(client);
    save->hunger = client_hunger(client);
    save->thirst = client_thirst(client);
    save->drunkeness = client_drunkeness(client);
    save->guildId = client_guild_id(client);
    save->x = mob_x(mob);
    save->y = mob_y(mob);
    save->z = mob_z(mob);
    save->heading = client_loc_heading(client);
    save->coin = *client_coin(client);
    save->coinCursor = *client_coin_cursor(client);
    save->coinBank = *client_coin_bank(client);
    save->materialIds[0] = mob_material_id(mob, 0);
    save->materialIds[1] = mob_material_id(mob, 1);
    save->materialIds[2] = mob_material_id(mob, 2);
    save->materialIds[3] = mob_material_id(mob, 3);
    save->materialIds[4] = mob_material_id(mob, 4);
    save->materialIds[5] = mob_material_id(mob, 5);
    save->materialIds[6] = mob_material_id(mob, 6);
    save->materialIds[7] = mob_material_id(mob, 7);
    save->materialIds[8] = mob_material_id(mob, 8);
    save->tints[0] = mob_tint(mob, 0);
    save->tints[1] = mob_tint(mob, 1);
    save->tints[2] = mob_tint(mob, 2);
    save->tints[3] = mob_tint(mob, 3);
    save->tints[4] = mob_tint(mob, 4);
    save->tints[5] = mob_tint(mob, 5);
    save->tints[6] = mob_tint(mob, 6);
    save->bindPoints[0] = *client_bind_point(client, 0);
    save->bindPoints[1] = *client_bind_point(client, 1);
    save->bindPoints[2] = *client_bind_point(client, 2);
    save->bindPoints[3] = *client_bind_point(client, 3);
    save->bindPoints[4] = *client_bind_point(client, 4);
    save->skills = *client_skills(client);

    for (i = 0; i < MEMMED_SPELL_SLOTS; i++)
    {
        save->memmedSpells[i].spellId = memmed[i].spellId;
        save->memmedSpells[i].recastTimestamp = memmed[i].recastTimestamp;
    }

    sbuf_grab(save->name);
    sbuf_grab(save->surname);

    rc = inv_save_all(inv, save);
    if (rc) goto fail;

    rc = spellbook_save_all(spells, save);
    if (rc) goto fail;

    save->zoneId = zone_id(zone);
    save->instId = zone_inst_id(zone);

    query.db.zQuery.qZoneClientSave.save = save;
    rc = db_queue_query(zone_db_queue(zone), NULL, zone_db_id(zone), 0, ZOP_DB_QueryZoneClientSave, &query);
    if (rc) goto fail;

    return;

fail:
    client_save_destroy(save);
}

void client_save_destroy(ClientSave* save)
{
    if (save)
    {
        sbuf_drop(save->surname);
        free_if_exists(save->items);
        free_if_exists(save->cursorQueue);
        free_if_exists(save->spellbook);
        free(save);
    }
}
