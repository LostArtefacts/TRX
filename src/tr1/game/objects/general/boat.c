#include "game/objects/general/boat.h"

#include "game/items.h"

void Boat_Setup(OBJECT *obj)
{
    obj->control = Boat_Control;
    obj->save_flags = 1;
    obj->save_anim = 1;
    obj->save_position = 1;
}

void Boat_Control(int16_t item_num)
{
    ITEM *item = &g_Items[item_num];

    switch (item->current_anim_state) {
    case BOAT_SET:
        item->goal_anim_state = BOAT_MOVE;
        break;
    case BOAT_MOVE:
        item->goal_anim_state = BOAT_STOP;
        break;
    case BOAT_STOP:
        Item_Kill(item_num);
        break;
    }

    Item_Animate(item);
}
