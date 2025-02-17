#include "game/items.h"
#include "global/const.h"

#include <libtrx/utils.h>

typedef enum {
    TRAPDOOR_STATE_CLOSED,
    TRAPDOOR_STATE_OPEN,
} TRAPDOOR_STATE;

static bool M_IsItemOnTop(const ITEM *item, int32_t x, int32_t z);
static void M_Setup(OBJECT *obj);
static void M_Control(int16_t item_num);
static int16_t M_GetFloorHeight(
    const ITEM *item, int32_t x, int32_t y, int32_t z, int16_t height);
static int16_t M_GetCeilingHeight(
    const ITEM *item, int32_t x, int32_t y, int32_t z, int16_t height);

static bool M_IsItemOnTop(const ITEM *item, int32_t x, int32_t z)
{
    const BOUNDS_16 *const orig_bounds = &Item_GetBestFrame(item)->bounds;
    BOUNDS_16 fixed_bounds = {};

    // Bounds need to change in order to account for 2 sector trapdoors
    // and the trapdoor angle.
    if (item->rot.y == 0) {
        fixed_bounds.min.x = orig_bounds->min.x;
        fixed_bounds.max.x = orig_bounds->max.x;
        fixed_bounds.min.z = orig_bounds->min.z;
        fixed_bounds.max.z = orig_bounds->max.z;
    } else if (item->rot.y == DEG_90) {
        fixed_bounds.min.x = orig_bounds->min.z;
        fixed_bounds.max.x = orig_bounds->max.z;
        fixed_bounds.min.z = -orig_bounds->max.x;
        fixed_bounds.max.z = -orig_bounds->min.x;
    } else if (item->rot.y == -DEG_180) {
        fixed_bounds.min.x = -orig_bounds->max.x;
        fixed_bounds.max.x = -orig_bounds->min.x;
        fixed_bounds.min.z = -orig_bounds->max.z;
        fixed_bounds.max.z = -orig_bounds->min.z;
    } else if (item->rot.y == -DEG_90) {
        fixed_bounds.min.x = -orig_bounds->max.z;
        fixed_bounds.max.x = -orig_bounds->min.z;
        fixed_bounds.min.z = orig_bounds->min.x;
        fixed_bounds.max.z = orig_bounds->max.x;
    }

    if (x <= item->pos.x + fixed_bounds.max.x
        && x >= item->pos.x + fixed_bounds.min.x
        && z <= item->pos.z + fixed_bounds.max.z
        && z >= item->pos.z + fixed_bounds.min.z) {
        return true;
    }

    return false;
}

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->floor_height_func = M_GetFloorHeight;
    obj->ceiling_height_func = M_GetCeilingHeight;
    obj->save_anim = 1;
    obj->save_flags = 1;
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (Item_IsTriggerActive(item)) {
        if (item->current_anim_state == TRAPDOOR_STATE_CLOSED) {
            item->goal_anim_state = TRAPDOOR_STATE_OPEN;
        }
    } else if (item->current_anim_state == TRAPDOOR_STATE_OPEN) {
        item->goal_anim_state = TRAPDOOR_STATE_CLOSED;
    }
    Item_Animate(item);
}

static int16_t M_GetFloorHeight(
    const ITEM *item, const int32_t x, const int32_t y, const int32_t z,
    const int16_t height)
{
    if (!M_IsItemOnTop(item, x, z)) {
        return height;
    }
    if (item->current_anim_state == TRAPDOOR_STATE_OPEN || y > item->pos.y
        || item->pos.y >= height) {
        return height;
    }
    return item->pos.y;
}

static int16_t M_GetCeilingHeight(
    const ITEM *item, const int32_t x, const int32_t y, const int32_t z,
    const int16_t height)
{
    if (!M_IsItemOnTop(item, x, z)) {
        return height;
    }
    if (item->current_anim_state == TRAPDOOR_STATE_OPEN || y <= item->pos.y
        || item->pos.y <= height) {
        return height;
    }
    return item->pos.y + STEP_L;
}

REGISTER_OBJECT(O_TRAPDOOR_1, M_Setup)
REGISTER_OBJECT(O_TRAPDOOR_2, M_Setup)
