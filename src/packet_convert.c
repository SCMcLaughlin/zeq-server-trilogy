
#include "packet_convert.h"
#include "class_id.h"
#include "item_client_type_id.h"
#include "item_type_id.h"
#include "race_id.h"

void pc_item_set_defaults(PC_Item* item)
{
    memset(item, 0, sizeof(PC_Item));
    
    item->itemType = ITEM_CLIENT_TYPE_Armor;
    item->model = 63;
    item->isPermanent = true;
    item->isDroppable = true;
    item->icon = 500;
    item->classes = 0xffff;
    item->races = 0xffff;
    item->spellId = 0xffff;
    item->clickySpellId = 0xffff;
}
