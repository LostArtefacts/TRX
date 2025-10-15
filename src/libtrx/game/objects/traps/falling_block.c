#include "game/lara.h"
#include "game/objects.h"
#include "game/objects/traps/movable_block.h"
#include "game/rooms.h"
#include "vector.h"
#include "version.h"

static int32_t M_GetOrigin(const OBJECT_ID obj_id)
{
    if (g_TRVersion == 1) {
        return -STEP_L * 2;
    } else {
        return obj_id == O_FALLING_BLOCK_3 ? -WALL_L : -STEP_L * 2;
    }
}

static void M_DropStack(const ITEM *const item)
{
    const int32_t origin = M_GetOrigin(item->object_id);
    XYZ_32 drop_pos = {
        .x = item->pos.x,
        .y = item->pos.y + origin,
        .z = item->pos.z,
    };
    MovableBlock_DropStack(drop_pos, item->room_num);
}

static int16_t M_GetFloorHeight(
    const ITEM *const item, const int32_t x, const int32_t y, const int32_t z,
    const int16_t height)
{
    const int32_t origin = M_GetOrigin(item->object_id);
    if (y <= item->pos.y + origin
        && (item->current_anim_state == TRAP_SET
            || item->current_anim_state == TRAP_ACTIVATE)) {
        return item->pos.y + origin;
    }
    return height;
}

static int16_t M_GetCeilingHeight(
    const ITEM *const item, const int32_t x, const int32_t y, const int32_t z,
    const int16_t height)
{
    const int32_t origin = M_GetOrigin(item->object_id);
    if (y > item->pos.y + origin
        && (item->current_anim_state == TRAP_SET
            || item->current_anim_state == TRAP_ACTIVATE)) {
        return item->pos.y + origin + STEP_L;
    }
    return height;
}

static void M_AddWalkable(const int16_t item_num)
{
    const ITEM *const item = Item_Get(item_num);
    Walkable_Add(item_num, item->pos);
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    const int32_t origin = M_GetOrigin(item->object_id);

    switch (item->current_anim_state) {
    case TRAP_SET:
        const ITEM *const lara_item = Lara_GetItem();
        if (lara_item->pos.y != item->pos.y + origin) {
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
            if (!item->gravity) {
                M_DropStack(item);
            }
            item->gravity = true;
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
    Item_UpdateRoom(item_num, room_num);

    item->floor = Room_GetHeight(sector, item->pos.x, item->pos.y, item->pos.z);

    if (item->current_anim_state == TRAP_WORKING
        && item->pos.y >= item->floor) {
        item->goal_anim_state = TRAP_FINISHED;
        item->pos.y = item->floor;
        item->fall_speed = 0;
        item->gravity = false;
    }
}

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->floor_height_func = M_GetFloorHeight;
    obj->ceiling_height_func = M_GetCeilingHeight;
    obj->add_walkable_func = M_AddWalkable;
    obj->save_position = true;
    obj->save_flags = true;
    obj->save_anim = true;
}

REGISTER_OBJECT(O_FALLING_BLOCK_1, M_Setup)
REGISTER_OBJECT(O_FALLING_BLOCK_2, M_Setup)
REGISTER_OBJECT(O_FALLING_BLOCK_3, M_Setup)
