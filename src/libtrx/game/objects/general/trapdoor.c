#include "game/const.h"
#include "game/objects.h"
#include "game/objects/traps/movable_block.h"
#include "log.h"

typedef enum {
    TRAPDOOR_STATE_CLOSED,
    TRAPDOOR_STATE_OPEN,
} TRAPDOOR_STATE;

// static void M_Initialise(const int16_t item_num);
// static void M_HandleSave(ITEM *const item, const SAVEGAME_STAGE stage);
static int16_t M_GetFloorHeight(
    const ITEM *item, int32_t x, int32_t y, int32_t z, int16_t height);
static int16_t M_GetCeilingHeight(
    const ITEM *item, int32_t x, int32_t y, int32_t z, int16_t height);
static bool M_IsItemOnTop(const ITEM *item, int32_t x, int32_t z);
static void M_Setup(OBJECT *obj);
static void M_Control(int16_t item_num);

// static void M_Initialise(const int16_t item_num)
// {
//     ITEM *const item = Item_Get(item_num);
//     if (item->current_anim_state == TRAPDOOR_STATE_OPEN) {
//         Item_RemoveWalkable(item_num);
//         LOG_DEBUG("INIT TRAPDOOR OPEN");
//     }
// }

// static void M_HandleSave(ITEM *const item, const SAVEGAME_STAGE stage)
// {
//     if (stage == SAVEGAME_STAGE_AFTER_LOAD) {
//         const int16_t item_num = Item_GetIndex(item);
//         if (item->current_anim_state == TRAPDOOR_STATE_OPEN) {
//             Item_RemoveWalkable(item_num);
//             LOG_DEBUG("LOAD TRAPDOOR OPEN");
//         }
//     }
// }

static int16_t M_GetFloorHeight(
    const ITEM *const item, const int32_t x, const int32_t y, const int32_t z,
    const int16_t height)
{
    // LOG_DEBUG(
    //     "item xyz: %d %d %d; test xyz: %d %d %d; height: %d", item->pos.x,
    //     item->pos.y, item->pos.z, x, y, z, height);

    if (!M_IsItemOnTop(item, x, z)) {
        // LOG_DEBUG("not on top");
        return height;
    } else if (item->current_anim_state != TRAPDOOR_STATE_CLOSED) {
        // LOG_DEBUG("not closed");
        return height;
    } else if (y > item->pos.y || item->pos.y > height) {
        // LOG_DEBUG("either or");
        return height;
    } else {
        // LOG_DEBUG("ON TRAPDOOR");
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
            MovableBlock_ActivateStack(
                item, item->pos.x, item->pos.y, item->pos.z);
        }
    } else {
        if (item->current_anim_state == TRAPDOOR_STATE_OPEN) {
            item->goal_anim_state = TRAPDOOR_STATE_CLOSED;
            // TODO Needed? Floor height functions should take care?
            // Item_AddWalkable(item_num);
            // Item_SortWalkables();
            MovableBlock_ActivateStack(
                item, item->pos.x, item->pos.y, item->pos.z);
        }
    }
    Item_Animate(item);
}

REGISTER_OBJECT(O_TRAPDOOR_TYPE_1, M_Setup)
REGISTER_OBJECT(O_TRAPDOOR_TYPE_2, M_Setup)
