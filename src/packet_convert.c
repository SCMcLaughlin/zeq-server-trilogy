
#include "packet_convert.h"
#include "class_id.h"
#include "item_type_id.h"
#include "race_id.h"

void pc_item_set_defaults(PC_Item* item)
{
    memset(item, 0, sizeof(PC_Item));
    
    item->itemType = ITEM_TYPE_Generic;
    item->model = 63;
    item->isPermanent = true;
    item->isDroppable = true;
    item->icon = 500;
    item->classes = CLASS_BIT_ALL;
    item->races = RACE_BIT_ALL;
}
