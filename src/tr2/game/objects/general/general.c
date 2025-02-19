#include "game/objects/general/general.h"

#include "game/collide.h"
#include "game/items.h"
#include "game/objects/common.h"
#include "game/output.h"
#include "game/room.h"

typedef enum {
    // clang-format off
    GENERAL_STATE_INACTIVE = 0,
    GENERAL_STATE_ACTIVE = 1,
    // clang-format on
} GENERAL_STATE;

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = General_Control;
    obj->collision_func = Object_Collision;
    obj->save_flags = 1;
    obj->save_anim = 1;
}

void General_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    if (Item_IsTriggerActive(item)) {
        item->goal_anim_state = GENERAL_STATE_ACTIVE;
    } else {
        item->goal_anim_state = GENERAL_STATE_INACTIVE;
    }
    Item_Animate(item);

    int16_t room_num = item->room_num;
    Room_GetSector(item->pos.x, item->pos.y, item->pos.z, &room_num);
    if (room_num != item->room_num) {
        Item_NewRoom(item_num, room_num);
    }

    XYZ_32 pos = { .x = 3000, .y = 720, .z = 0 };
    Collide_GetJointAbsPosition(item, &pos, 0);
    Output_AddDynamicLight(pos, 14, 11);

    if (item->status == IS_DEACTIVATED) {
        Item_RemoveActive(item_num);
        item->flags |= IF_ONE_SHOT;
    }
}

REGISTER_OBJECT(O_GENERAL, M_Setup)
