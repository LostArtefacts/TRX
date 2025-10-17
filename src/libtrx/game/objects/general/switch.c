#include "game/objects/general/switch.h"

#include "config.h"
#include "game/const.h"
#include "game/items.h"
#include "game/objects.h"

static const OBJECT_BOUNDS m_SwitchBounds = {
    .shift = {
        .min = { .x = -220, .y = +0, .z = +WALL_L / 2 - 220, },
        .max = { .x = +220, .y = +0, .z = +WALL_L / 2, },
    },
    .rot = {
        .min = { .x = -10 * DEG_1, .y = -30 * DEG_1, .z = -10 * DEG_1, },
        .max = { .x = +10 * DEG_1, .y = +30 * DEG_1, .z = +10 * DEG_1, },
    },
};

static OBJECT_BOUNDS m_SwitchBoundsControlled = {
    .shift = {
        .min = { .x = +0, .y = +0, .z = +0, },
        .max = { .x = +0, .y = +0, .z = +0, },
    },
    .rot = {
        .min = { .x = -10 * DEG_1, .y = -30 * DEG_1, .z = -10 * DEG_1, },
        .max = { .x = +10 * DEG_1, .y = +30 * DEG_1, .z = +10 * DEG_1, },
    },
};

static const OBJECT_BOUNDS m_SwitchBoundsUW = {
    .shift = {
        .min = { .x = -WALL_L, .y = -WALL_L, .z = -WALL_L, },
        .max = { .x = +WALL_L, .y = +WALL_L, .z = +WALL_L / 2, },
    },
    .rot = {
        .min = { .x = -80 * DEG_1, .y = -80 * DEG_1, .z = -80 * DEG_1, },
        .max = { .x = +80 * DEG_1, .y = +80 * DEG_1, .z = +80 * DEG_1, },
    },
};

static const OBJECT_BOUNDS *M_Bounds(void)
{
#if TR_VERSION == 1
    if (g_Config.gameplay.enable_walk_to_items) {
        return &m_SwitchBoundsControlled;
    }
#endif
    return &m_SwitchBounds;
}

static const OBJECT_BOUNDS *M_BoundsUW(void)
{
    return &m_SwitchBoundsUW;
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    item->flags |= IF_CODE_BITS;
    if (!Item_IsTriggerActive(item)) {
        item->goal_anim_state = SWITCH_STATE_ON;
        item->timer = 0;
    }
    Item_Animate(item);
}

static void M_SetupBase(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->save_flags = true;
    obj->save_anim = true;
}

static void M_Setup(OBJECT *const obj)
{
    M_SetupBase(obj);
    obj->collision_func = Switch_Collision;
    obj->bounds_func = M_Bounds;
}

static void M_SetupPushButton(OBJECT *const obj)
{
    M_Setup(obj);
    obj->enable_interpolation = false;
    obj->bounds_func = M_Bounds;
}

static void M_SetupUW(OBJECT *const obj)
{
    M_SetupBase(obj);
    obj->collision_func = Switch_CollisionUW;
    obj->bounds_func = M_BoundsUW;
}

bool Switch_Trigger(const int16_t item_num, const int16_t timer)
{
    ITEM *const item = Item_Get(item_num);

    if (item->object_id == O_SWITCH_TYPE_AIRLOCK) {
        if (item->status == IS_DEACTIVATED) {
            Item_RemoveActive(item_num);
            item->status = IS_INACTIVE;
            return false;
        } else if (
            (item->flags & IF_ONE_SHOT) != 0
            || item->current_anim_state == SWITCH_STATE_OFF) {
            return false;
        }

        item->flags |= IF_ONE_SHOT;
        return true;
    }

    if (item->status != IS_DEACTIVATED) {
        return false;
    }

    if (item->current_anim_state == SWITCH_STATE_OFF && timer > 0) {
        item->timer = timer;
        if (timer != 1) {
            item->timer *= LOGIC_FPS;
        }
        item->status = IS_ACTIVE;
    } else {
        Item_RemoveActive(item_num);
        item->status = IS_INACTIVE;
    }
    return true;
}

REGISTER_OBJECT(O_SWITCH_TYPE_AIRLOCK, M_Setup)
REGISTER_OBJECT(O_SWITCH_TYPE_BUTTON, M_SetupPushButton)
REGISTER_OBJECT(O_SWITCH_TYPE_NORMAL, M_Setup)
REGISTER_OBJECT(O_SWITCH_TYPE_SMALL, M_Setup)
REGISTER_OBJECT(O_SWITCH_TYPE_UW, M_SetupUW)
