#include "game/objects/traps/falling_block.h"

#include "game/items.h"
#include "game/room.h"
#include "global/const.h"
#include "global/vars.h"

void FallingBlock_Setup(OBJECT *obj)
{
    obj->control = FallingBlock_Control;
    obj->floor_height_func = FallingBlock_GetFloorHeight;
    obj->ceiling_height_func = FallingBlock_GetCeilingHeight;
    obj->save_position = 1;
    obj->save_anim = 1;
    obj->save_flags = 1;
}

void FallingBlock_Control(int16_t item_num)
{
    ITEM *item = &g_Items[item_num];

    switch (item->current_anim_state) {
    case TRAP_SET:
        if (g_LaraItem->pos.y == item->pos.y - STEP_L * 2) {
            item->goal_anim_state = TRAP_ACTIVATE;
        } else {
            item->status = IS_INACTIVE;
            Item_RemoveActive(item_num);
            return;
        }
        break;

    case TRAP_ACTIVATE:
        item->goal_anim_state = TRAP_WORKING;
        break;

    case TRAP_WORKING:
        if (item->goal_anim_state != TRAP_FINISHED) {
            item->gravity = 1;
        }
        break;
    }

    Item_Animate(item);
    if (item->status == IS_DEACTIVATED) {
        Item_RemoveActive(item_num);
        return;
    }

    int16_t room_num = item->room_num;
    const SECTOR *const sector =
        Room_GetSector(item->pos.x, item->pos.y, item->pos.z, &room_num);
    if (item->room_num != room_num) {
        Item_NewRoom(item_num, room_num);
    }

    item->floor = Room_GetHeight(sector, item->pos.x, item->pos.y, item->pos.z);

    if (item->current_anim_state == TRAP_WORKING
        && item->pos.y >= item->floor) {
        item->goal_anim_state = TRAP_FINISHED;
        item->pos.y = item->floor;
        item->fall_speed = 0;
        item->gravity = 0;
    }
}

int16_t FallingBlock_GetFloorHeight(
    const ITEM *item, const int32_t x, const int32_t y, const int32_t z,
    const int16_t height)
{
    if (y <= item->pos.y - STEP_L * 2
        && (item->current_anim_state == TRAP_SET
            || item->current_anim_state == TRAP_ACTIVATE)) {
        return item->pos.y - STEP_L * 2;
    }

    return height;
}

int16_t FallingBlock_GetCeilingHeight(
    const ITEM *item, const int32_t x, const int32_t y, const int32_t z,
    const int16_t height)
{
    if (y > item->pos.y - STEP_L * 2
        && (item->current_anim_state == TRAP_SET
            || item->current_anim_state == TRAP_ACTIVATE)) {
        return item->pos.y - STEP_L;
    }

    return height;
}
