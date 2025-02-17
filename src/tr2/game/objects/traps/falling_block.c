#include "game/objects/traps/falling_block.h"

#include "game/items.h"
#include "game/room.h"
#include "global/vars.h"

static int32_t M_GetOrigin(GAME_OBJECT_ID obj_id);
static int16_t M_GetFloorHeight(
    const ITEM *item, int32_t x, int32_t y, int32_t z, int16_t height);
static int16_t M_GetCeilingHeight(
    const ITEM *item, int32_t x, int32_t y, int32_t z, int16_t height);

static int32_t M_GetOrigin(const GAME_OBJECT_ID obj_id)
{
    return obj_id == O_FALLING_BLOCK_3 ? WALL_L : STEP_L * 2;
}

static int16_t M_GetFloorHeight(
    const ITEM *const item, const int32_t x, const int32_t y, const int32_t z,
    const int16_t height)
{
    const int32_t origin = M_GetOrigin(item->object_id);
    if (y <= item->pos.y - origin
        && (item->current_anim_state == TRAP_SET
            || item->current_anim_state == TRAP_ACTIVATE)) {
        return item->pos.y - origin;
    }
    return height;
}

static int16_t M_GetCeilingHeight(
    const ITEM *const item, const int32_t x, const int32_t y, const int32_t z,
    const int16_t height)
{
    const int32_t origin = M_GetOrigin(item->object_id);
    if (y > item->pos.y - origin
        && (item->current_anim_state == TRAP_SET
            || item->current_anim_state == TRAP_ACTIVATE)) {
        return item->pos.y - origin + STEP_L;
    }
    return height;
}

void FallingBlock_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    const int32_t origin = M_GetOrigin(item->object_id);

    switch (item->current_anim_state) {
    case TRAP_SET:
        if (g_LaraItem->pos.y != item->pos.y - origin) {
            item->status = IS_INACTIVE;
            Item_RemoveActive(item_num);
            return;
        }
        item->goal_anim_state = TRAP_ACTIVATE;
        break;

    case TRAP_ACTIVATE:
        item->goal_anim_state = TRAP_WORKING;
        break;

    case TRAP_WORKING:
        if (item->goal_anim_state != TRAP_FINISHED) {
            item->gravity = 1;
        }
        break;

    default:
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

void FallingBlock_Setup(OBJECT *const obj)
{
    obj->control_func = FallingBlock_Control;
    obj->floor_height_func = M_GetFloorHeight;
    obj->ceiling_height_func = M_GetCeilingHeight;
    obj->save_position = 1;
    obj->save_flags = 1;
    obj->save_anim = 1;
}
