#include "game/lara.h"
#include "game/objects/common.h"

#include <libtrx/config.h>
#include <libtrx/game/input.h>
#include <libtrx/game/objects/general/switch.h>

static const OBJECT_BOUNDS m_Switch_Bounds = {
    .shift = {
        .min = { .x = -200, .y = +0, .z = +WALL_L / 2 - 200, },
        .max = { .x = +200, .y = +0, .z = +WALL_L / 2, },
    },
    .rot = {
        .min = { .x = -10 * DEG_1, .y = -30 * DEG_1, .z = -10 * DEG_1, },
        .max = { .x = +10 * DEG_1, .y = +30 * DEG_1, .z = +10 * DEG_1, },
    },
};

static OBJECT_BOUNDS m_Switch_BoundsControlled = {
    .shift = {
        .min = { .x = +0, .y = +0, .z = +0, },
        .max = { .x = +0, .y = +0, .z = +0, },
    },
    .rot = {
        .min = { .x = -10 * DEG_1, .y = -30 * DEG_1, .z = -10 * DEG_1, },
        .max = { .x = +10 * DEG_1, .y = +30 * DEG_1, .z = +10 * DEG_1, },
    },
};

static const OBJECT_BOUNDS m_Switch_BoundsUW = {
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
    if (g_Config.gameplay.enable_walk_to_items) {
        return &m_Switch_BoundsControlled;
    }
    return &m_Switch_Bounds;
}

static const OBJECT_BOUNDS *M_BoundsUW(void)
{
    return &m_Switch_BoundsUW;
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

static void M_CollisionControlled(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);
    LARA_INFO *const lara = Lara_GetLaraInfo();

    if ((g_Input.action && lara->gun_status == LGS_ARMLESS
         && !lara_item->gravity && lara_item->current_anim_state == LS(LS_STOP)
         && item->status == IS_INACTIVE)
        || (lara->interact_target.is_moving
            && lara->interact_target.item_num == item_num)) {
        const BOUNDS_16 *const bounds = Item_GetBoundsAccurate(item);

        m_Switch_BoundsControlled.shift.min.x = bounds->min.x - 256;
        m_Switch_BoundsControlled.shift.max.x = bounds->max.x + 256;
        m_Switch_BoundsControlled.shift.min.z = bounds->min.z - 200;
        m_Switch_BoundsControlled.shift.max.z = bounds->max.z + 200;

        XYZ_32 move_vector = { 0, 0, bounds->min.z - 64 };

        if (Lara_TestPosition(item, &m_Switch_BoundsControlled)) {
            if (Lara_MovePosition(item, &move_vector)) {
                if (item->current_anim_state == SWITCH_STATE_ON) {
                    Item_SwitchToAnim(lara_item, LA(LA_WALL_SWITCH_DOWN), 0);
                    lara_item->current_anim_state = LS(LS_SWITCH_OFF);
                    item->goal_anim_state = SWITCH_STATE_OFF;
                } else {
                    Item_SwitchToAnim(lara_item, LA(LA_WALL_SWITCH_UP), 0);
                    lara_item->current_anim_state = LS(LS_SWITCH_ON);
                    item->goal_anim_state = SWITCH_STATE_ON;
                }
                lara->head_rot.x = 0;
                lara->head_rot.y = 0;
                lara->torso_rot.x = 0;
                lara->torso_rot.y = 0;
                lara->interact_target.is_moving = false;
                lara->interact_target.item_num = NO_ITEM;
                lara->gun_status = LGS_HANDS_BUSY;
                Item_AddActive(item_num);
                item->status = IS_ACTIVE;
                Item_Animate(item);
            } else {
                lara->interact_target.item_num = item_num;
            }
        } else if (
            lara->interact_target.is_moving
            && lara->interact_target.item_num == item_num) {
            lara->interact_target.is_moving = false;
            lara->gun_status = LGS_ARMLESS;
        }
    } else if (
        lara_item->current_anim_state != LS(LS_SWITCH_ON)
        && lara_item->current_anim_state != LS(LS_SWITCH_OFF)) {
        Object_Collision(item_num, lara_item, coll);
    }
}

static void M_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    if (g_Config.gameplay.enable_walk_to_items) {
        M_CollisionControlled(item_num, lara_item, coll);
        return;
    }

    ITEM *const item = Item_Get(item_num);
    LARA_INFO *const lara = Lara_GetLaraInfo();
    const OBJECT *const obj = Object_Get(item->object_id);

    if (!g_Input.action || item->status != IS_INACTIVE
        || lara->gun_status != LGS_ARMLESS || lara_item->gravity) {
        return;
    }

    if (lara_item->current_anim_state != LS(LS_STOP)) {
        return;
    }

    if (!Lara_TestPosition(item, obj->bounds_func())) {
        return;
    }

    lara_item->rot.y = item->rot.y;
    if (item->current_anim_state == SWITCH_STATE_ON) {
        Lara_AnimateUntil(lara_item, LS(LS_SWITCH_ON));
        lara_item->goal_anim_state = LS(LS_STOP);
        lara->gun_status = LGS_HANDS_BUSY;
        item->status = IS_ACTIVE;
        item->goal_anim_state = SWITCH_STATE_OFF;
        Item_AddActive(item_num);
        Item_Animate(item);
    } else if (item->current_anim_state == SWITCH_STATE_OFF) {
        Lara_AnimateUntil(lara_item, LS(LS_SWITCH_OFF));
        lara_item->goal_anim_state = LS(LS_STOP);
        lara->gun_status = LGS_HANDS_BUSY;
        item->status = IS_ACTIVE;
        item->goal_anim_state = SWITCH_STATE_ON;
        Item_AddActive(item_num);
        Item_Animate(item);
    }
}

static void M_CollisionUW(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);
    LARA_INFO *const lara = Lara_GetLaraInfo();
    const OBJECT *const obj = Object_Get(item->object_id);

    if (!g_Input.action || item->status != IS_INACTIVE
        || (lara->water_status != LWS_UNDERWATER
            && lara->water_status != LWS_CHEAT)) {
        return;
    }

    if (lara_item->current_anim_state != LS(LS_TREAD)) {
        return;
    }

    if (!Lara_TestPosition(item, obj->bounds_func())) {
        return;
    }

    if (item->current_anim_state == SWITCH_STATE_ON
        || item->current_anim_state == SWITCH_STATE_OFF) {
        XYZ_32 move_vector_uw = { 0, 0, 108 };
        if (!Lara_MovePosition(item, &move_vector_uw)) {
            return;
        }
        lara_item->fall_speed = 0;
        Lara_AnimateUntil(lara_item, LS(LS_SWITCH_ON));
        lara_item->goal_anim_state = LS(LS_TREAD);
        lara->gun_status = LGS_HANDS_BUSY;
        item->status = IS_ACTIVE;
        if (item->current_anim_state == SWITCH_STATE_ON) {
            item->goal_anim_state = SWITCH_STATE_OFF;
        } else {
            item->goal_anim_state = SWITCH_STATE_ON;
        }
        Item_AddActive(item_num);
        Item_Animate(item);
    }
}

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->collision_func = M_Collision;
    obj->save_anim = true;
    obj->save_flags = true;
    obj->bounds_func = M_Bounds;
}

static void M_SetupUW(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->collision_func = M_CollisionUW;
    obj->save_anim = true;
    obj->save_flags = true;
    obj->bounds_func = M_BoundsUW;
}

bool Switch_Trigger(int16_t item_num, int16_t timer)
{
    ITEM *const item = Item_Get(item_num);
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

REGISTER_OBJECT(O_SWITCH_TYPE_NORMAL, M_Setup)
REGISTER_OBJECT(O_SWITCH_TYPE_UW, M_SetupUW)
