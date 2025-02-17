#include "game/objects/traps/falling_ceiling.h"

#include "game/items.h"
#include "game/lara/control.h"
#include "game/objects/common.h"
#include "game/room.h"

#define FALLING_CEILING_DAMAGE 300

void FallingCeiling_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    if (item->current_anim_state == TRAP_SET) {
        item->goal_anim_state = TRAP_ACTIVATE;
        item->gravity = 1;
    } else if (
        item->current_anim_state == TRAP_ACTIVATE && item->touch_bits != 0) {
        Lara_TakeDamage(FALLING_CEILING_DAMAGE, true);
    }

    Item_Animate(item);
    if (item->status == IS_DEACTIVATED) {
        Item_RemoveActive(item_num);
        return;
    }

    int16_t room_num = item->room_num;
    const SECTOR *const sector =
        Room_GetSector(item->pos.x, item->pos.y, item->pos.z, &room_num);
    const int16_t height =
        Room_GetHeight(sector, item->pos.x, item->pos.y, item->pos.z);

    item->floor = height;
    if (room_num != item->room_num) {
        Item_NewRoom(item_num, room_num);
    }
    if (item->current_anim_state == TRAP_ACTIVATE && item->pos.y >= height) {
        item->pos.y = height;
        item->goal_anim_state = TRAP_WORKING;
        item->fall_speed = 0;
        item->gravity = 0;
    }
}

void FallingCeiling_Setup(void)
{
    OBJECT *const obj = Object_Get(O_FALLING_CEILING);
    obj->control_func = FallingCeiling_Control;
    obj->collision_func = Object_Collision_Trap;
    obj->save_position = 1;
    obj->save_flags = 1;
    obj->save_anim = 1;
}
