
#include "mob.h"
#include "buffer.h"
#include "client_load_data.h"
#include "loc.h"
#include "misc_enum.h"
#include "misc_struct.h"
#include "race_id.h"
#include "zone.h"

void mob_init_client_unloaded(Mob* mob, StaticBuffer* name)
{
    mob->parentType = MOB_PARENT_TYPE_Client;
    mob->name = name;
    sbuf_grab(name);
}

static float mob_base_size_by_race_id(int raceId)
{
    float ret;
    
    switch (raceId)
    {
    case RACE_ID_Barbarian:
    case RACE_ID_Ogre:
    case RACE_ID_Troll:
        ret = 6.0f;
        break;
    
    case RACE_ID_WoodElf:
    case RACE_ID_DarkElf:
        ret = 4.0f;
        break;
    
    case RACE_ID_Dwarf:
    case RACE_ID_Halfling:
        ret = 3.0f;
        break;
    
    case RACE_ID_Gnome:
        ret = 2.5f;
        break;
    
    default:
        ret = 5.0f;
        break;
    }
    
    return ret;
}

void mob_init_client_character(Mob* mob, ClientLoadData_Character* data)
{
    float size;
    float runSpeed;
    
    mob->level = data->level;
    mob->classId = data->classId;
    mob->baseGenderId = data->genderId;
    mob->genderId = data->genderId;
    mob->faceId = data->faceId;
    mob->baseRaceId = data->raceId;
    mob->raceId = data->raceId;
    mob->deityId = data->deityId;
    mob->zoneIndex = -1;
    mob->loc = data->loc;
    mob_set_cur_hp_no_cap_check(mob, data->currentHp);
    mob_set_cur_mana_no_cap_check(mob, data->currentMana);
    mob_set_cur_endurance_no_cap_check(mob, data->currentEndurance);
    mob->baseStats.STR = data->STR;
    mob->baseStats.STA = data->STA;
    mob->baseStats.DEX = data->DEX;
    mob->baseStats.AGI = data->AGI;
    mob->baseStats.INT = data->INT;
    mob->baseStats.WIS = data->WIS;
    mob->baseStats.CHA = data->CHA;
    
    mob->textureId = 0xff;
    mob->helmTextureId = 0xff;
    mob->primaryWeaponModelId = 0;
    mob->secondaryWeaponModelId = 0;
    mob->uprightState = UPRIGHT_STATE_Standing;
    mob->lightLevel = 0;
    mob->bodyType = BODY_TYPE_Humanoid;
    
    /* Temporary, just to make sure we don't try to divide by zero anywhere */
    mob->baseStats.maxHp = 100;
    mob->baseStats.maxMana = 100;
    mob->baseStats.maxEndurance = 100;
    mob->cappedStats.maxHp = 100;
    mob->cappedStats.maxMana = 100;
    mob->cappedStats.maxEndurance = 100;
    
    runSpeed = (data->isGMSpeed) ? 3.0f : 0.7f;
    
    mob->currentWalkingSpeed = 0.46f;
    mob->currentRunningSpeed = runSpeed;
    mob->baseWalkingSpeed = 0.46f;
    mob->baseRunningSpeed = runSpeed;
    
    size = mob_base_size_by_race_id((int)mob->baseRaceId);
    
    mob->currentSize = size;
    mob->baseSize = size;
}

void mob_deinit(Mob* mob)
{
    mob->name = sbuf_drop(mob->name);
}

int8_t mob_hp_ratio(Mob* mob)
{
    return (int8_t)((mob->currentHp * 100) / mob->cappedStats.maxHp);
}
