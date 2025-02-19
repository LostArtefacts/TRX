#include "game/items.h"
#include "game/objects/common.h"
#include "game/room.h"
#include "global/vars.h"

typedef enum {
    BELL_STATE_STOP = 0,
    BELL_STATE_SWING = 1,
} BELL_STATE;

static void M_Setup(OBJECT *obj);
static void M_Control(int16_t item_num);

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->collision_func = Object_Collision;
    obj->save_flags = 1;
    obj->save_anim = 1;
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    item->goal_anim_state = BELL_STATE_SWING;

    const SECTOR *const sector =
        Room_GetSector(item->pos.x, item->pos.y, item->pos.z, &item->room_num);
    item->floor = Room_GetHeight(sector, item->pos.x, item->pos.y, item->pos.z);
    Room_TestTriggers(item);

    Item_Animate(item);

    if (item->current_anim_state == BELL_STATE_STOP) {
        item->status = IS_INACTIVE;
        Item_RemoveActive(item_num);
    }
}

REGISTER_OBJECT(O_BELL, M_Setup)
