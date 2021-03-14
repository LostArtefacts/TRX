#include "game/objects/cog.h"

#include "game/control.h"
#include "game/items.h"
#include "game/vars.h"

void SetupCog(OBJECT_INFO *obj)
{
    obj->control = CogControl;
    obj->save_flags = 1;
}

void CogControl(int16_t item_num)
{
    ITEM_INFO *item = &Items[item_num];
    if (TriggerActive(item)) {
        item->goal_anim_state = DOOR_OPEN;
    } else {
        item->goal_anim_state = DOOR_CLOSED;
    }

    AnimateItem(item);

    int16_t room_num = item->room_number;
    GetFloor(item->pos.x, item->pos.y, item->pos.z, &room_num);
    if (room_num != item->room_number) {
        ItemNewRoom(item_num, room_num);
    }
}
