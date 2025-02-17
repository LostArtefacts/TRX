#include "game/objects/general/cog.h"

#include "game/items.h"
#include "game/room.h"

typedef enum {
    COG_STATE_INACTIVE = 0,
    COG_STATE_ACTIVE = 1,
} COG_STATE;

void Cog_Setup(OBJECT *obj)
{
    obj->control_func = Cog_Control;
    obj->save_flags = 1;
}

void Cog_Control(int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (Item_IsTriggerActive(item)) {
        item->goal_anim_state = COG_STATE_ACTIVE;
    } else {
        item->goal_anim_state = COG_STATE_INACTIVE;
    }

    Item_Animate(item);

    int16_t room_num = item->room_num;
    Room_GetSector(item->pos.x, item->pos.y, item->pos.z, &room_num);
    if (room_num != item->room_num) {
        Item_NewRoom(item_num, room_num);
    }
}
