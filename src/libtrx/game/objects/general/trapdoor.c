#include "game/const.h"
#include "game/objects.h"
#include "game/objects/traps/movable_block.h"
#include "log.h"
#include "utils.h"

typedef enum {
    TRAPDOOR_STATE_CLOSED,
    TRAPDOOR_STATE_OPEN,
} TRAPDOOR_STATE;

static int16_t M_GetFloorHeight(
    const ITEM *item, int32_t x, int32_t y, int32_t z, int16_t height);
static int16_t M_GetCeilingHeight(
    const ITEM *item, int32_t x, int32_t y, int32_t z, int16_t height);
static bool M_IsItemOnTop(const ITEM *item, int32_t x, int32_t z);
static void M_Setup(OBJECT *obj);
static void M_Control(int16_t item_num);

static void M_ActivateSectors(const ITEM *item)
{
    const BOUNDS_16 *orig_bounds = &Item_GetBestFrame(item)->bounds;
    if (!orig_bounds)
        return;

    // Rotate bounds to the correct x and z.
    LOG_DEBUG("item->rot.y: %d", item->rot.y);
    BOUNDS_16 rot_bounds;
    int32_t x_walk = 0;
    int32_t z_walk = 0;
    int32_t min_x = item->pos.x;
    int32_t max_x = item->pos.x;
    int32_t min_z = item->pos.z;
    int32_t max_z = item->pos.z;
    switch (item->rot.y) {
    case 0:
        rot_bounds = *orig_bounds;
        z_walk = WALL_L;
        max_z = item->pos.z + (rot_bounds.min.z + rot_bounds.max.z);
        break;
    case DEG_90:
        rot_bounds.min.x = orig_bounds->min.z;
        rot_bounds.max.x = orig_bounds->max.z;
        rot_bounds.min.z = -orig_bounds->max.x;
        rot_bounds.max.z = -orig_bounds->min.x;
        x_walk = WALL_L;
        max_x = item->pos.x + (rot_bounds.min.x + rot_bounds.max.x);
        break;
    case -DEG_180:
        rot_bounds.min.x = -orig_bounds->max.x;
        rot_bounds.max.x = -orig_bounds->min.x;
        rot_bounds.min.z = -orig_bounds->max.z;
        rot_bounds.max.z = -orig_bounds->min.z;
        z_walk = -WALL_L;
        max_z = item->pos.z + (rot_bounds.min.z + rot_bounds.max.z);
        LOG_DEBUG(
            "z pos: %d; bounds max z: %d %d", item->pos.z, rot_bounds.min.z,
            rot_bounds.max.z);
        break;
    case -DEG_90:
        rot_bounds.min.x = -orig_bounds->max.z;
        rot_bounds.max.x = -orig_bounds->min.z;
        rot_bounds.min.z = orig_bounds->min.x;
        rot_bounds.max.z = orig_bounds->max.x;
        x_walk = -WALL_L;
        max_x = item->pos.x + (rot_bounds.min.x + rot_bounds.max.x);
        break;
    default:
        rot_bounds = *orig_bounds;
        break;
    }

    // Swap min and max for loop if traverse in a negative direction.
    if (x_walk == -WALL_L) {
        int32_t t = min_x;
        min_x = max_x;
        max_x = t;
    }
    if (z_walk == -WALL_L) {
        int32_t t = min_z;
        min_z = max_z;
        max_z = t;
    }

    LOG_DEBUG("swapped min max x: %d %d; z: %d %d", min_x, max_x, min_z, max_z);

    // Walk every covered sector.
    for (int32_t x = min_x; x <= max_x; x += WALL_L) {
        for (int32_t z = min_z; z <= max_z; z += WALL_L) {
            LOG_DEBUG("Trigger xz: %d %d", x, z);
            const XYZ_32 sector_pos = {
                .x = x,
                .y = item->pos.y,
                .z = z,
            };
            MovableBlock_ActivateStack(item, sector_pos);
        }
    }
}

static int16_t M_GetFloorHeight(
    const ITEM *const item, const int32_t x, const int32_t y, const int32_t z,
    const int16_t height)
{

    if (!M_IsItemOnTop(item, x, z)) {
        return height;
    } else if (item->current_anim_state != TRAPDOOR_STATE_CLOSED) {
        return height;
    } else if (y > item->pos.y || item->pos.y > height) {
        return height;
    } else {
        return item->pos.y;
    }
}

static int16_t M_GetCeilingHeight(
    const ITEM *const item, const int32_t x, const int32_t y, const int32_t z,
    const int16_t height)
{
    if (!M_IsItemOnTop(item, x, z)) {
        return height;
    } else if (item->current_anim_state != TRAPDOOR_STATE_CLOSED) {
        return height;
    } else if (y <= item->pos.y || item->pos.y <= height) {
        return height;
    } else {
        return item->pos.y + STEP_L;
    }
}

static bool M_IsItemOnTop(
    const ITEM *const item, const int32_t x, const int32_t z)
{
    const BOUNDS_16 *const orig_bounds = &Item_GetBestFrame(item)->bounds;
    if (orig_bounds == nullptr) {
        return false;
    }

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
    // obj->initialise_func = M_Initialise;
    // obj->handle_save_func = M_HandleSave;
    obj->control_func = M_Control;
    obj->floor_height_func = M_GetFloorHeight;
    obj->ceiling_height_func = M_GetCeilingHeight;
    obj->save_flags = true;
    obj->save_anim = true;
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (Item_IsTriggerActive(item)) {
        if (item->current_anim_state == TRAPDOOR_STATE_CLOSED) {
            item->goal_anim_state = TRAPDOOR_STATE_OPEN;
            // TODO Needed? Floor height functions should take care?
            // Item_RemoveWalkable(item_num);
            // Item_SortWalkables();
            LOG_DEBUG("Activate trapdoor item_num: %d", item_num);
            M_ActivateSectors(item);
        }
    } else {
        if (item->current_anim_state == TRAPDOOR_STATE_OPEN) {
            item->goal_anim_state = TRAPDOOR_STATE_CLOSED;
            // TODO Needed? Floor height functions should take care?
            // Item_AddWalkable(item_num);
            // Item_SortWalkables();
            // M_ActivateSectors(item);
        }
    }

    Item_Animate(item);
}

REGISTER_OBJECT(O_TRAPDOOR_TYPE_1, M_Setup)
REGISTER_OBJECT(O_TRAPDOOR_TYPE_2, M_Setup)
