#include "game/objects/general/secret.h"

#include "game/items.h"
#include "game/objects/common.h"
#include "game/objects/general/pickup.h"

void Secret2_Control(int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    item->status = IS_INVISIBLE;
    Item_RemoveDrawn(item_num);
}

void Secret2_Setup(void)
{
    OBJECT *const obj = Object_Get(O_SECRET_2);
    Pickup_Setup(obj);
    // TODO: why is it so special?
    obj->control_func = Secret2_Control;
}
