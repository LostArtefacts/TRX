#include <trx/game/lara.h>
#include <trx/game/objects.h>
#include <trx/game/objects/traps/movable_block.h>
#include <trx/game/rooms.h>
#include <trx/vector.h>

static int32_t M_GetOrigin(const ITEM *const item)
{
    return (int32_t)(intptr_t)item->priv;
}

static void M_CalculateOrigin(ITEM *const item)
{
    const OBJECT *const obj = Object_Get(item->object_id);
    const ANIM *const anim = Object_GetAnim(obj, 0);
    const ANIM_FRAME *const frame = &anim->frame_ptr[0];
    item->priv = (void *)(intptr_t)ROUND_TO_CLICK_SIGNED(frame->offset.y);
}

static void M_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_CalculateOrigin(item);
}

static void M_DropStack(const ITEM *const item)
{
    const int32_t origin = M_GetOrigin(item);
    const XYZ_32 drop_pos = {
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
    const int32_t origin = M_GetOrigin(item);
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
    const int32_t origin = M_GetOrigin(item);
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
    const int32_t origin = M_GetOrigin(item);

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
    obj->initialise_func = M_Initialise;
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
