#include "game/objects/general/switch.h"

#include "config.h"
#include "game/input.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/objects/common.h"
#include "global/const.h"
#include "global/vars.h"

static const OBJECT_BOUNDS m_Switch_Bounds = {
    .shift = {
        .min = { .x = -200, .y = +0, .z = +WALL_L / 2 - 200, },
        .max = { .x = +200, .y = +0, .z = +WALL_L / 2, },
    },
    .rot = {
        .min = { .x = -10 * PHD_DEGREE, .y = -30 * PHD_DEGREE, .z = -10 * PHD_DEGREE, },
        .max = { .x = +10 * PHD_DEGREE, .y = +30 * PHD_DEGREE, .z = +10 * PHD_DEGREE, },
    },
};

static OBJECT_BOUNDS m_Switch_BoundsControlled = {
    .shift = {
        .min = { .x = +0, .y = +0, .z = +0, },
        .max = { .x = +0, .y = +0, .z = +0, },
    },
    .rot = {
        .min = { .x = -10 * PHD_DEGREE, .y = -30 * PHD_DEGREE, .z = -10 * PHD_DEGREE, },
        .max = { .x = +10 * PHD_DEGREE, .y = +30 * PHD_DEGREE, .z = +10 * PHD_DEGREE, },
    },
};

static const OBJECT_BOUNDS m_Switch_BoundsUW = {
    .shift = {
        .min = { .x = -WALL_L, .y = -WALL_L, .z = -WALL_L, },
        .max = { .x = +WALL_L, .y = +WALL_L, .z = +WALL_L / 2, },
    },
    .rot = {
        .min = { .x = -80 * PHD_DEGREE, .y = -80 * PHD_DEGREE, .z = -80 * PHD_DEGREE, },
        .max = { .x = +80 * PHD_DEGREE, .y = +80 * PHD_DEGREE, .z = +80 * PHD_DEGREE, },
    },
};

static const OBJECT_BOUNDS *Switch_Bounds(void);
static const OBJECT_BOUNDS *Switch_BoundsUW(void);

static const OBJECT_BOUNDS *Switch_Bounds(void)
{
    if (g_Config.walk_to_items) {
        return &m_Switch_BoundsControlled;
    }
    return &m_Switch_Bounds;
}

static const OBJECT_BOUNDS *Switch_BoundsUW(void)
{
    return &m_Switch_BoundsUW;
}

void Switch_Setup(OBJECT_INFO *obj)
{
    obj->control = Switch_Control;
    obj->collision = Switch_Collision;
    obj->save_anim = 1;
    obj->save_flags = 1;
    obj->bounds = Switch_Bounds;
}

void Switch_SetupUW(OBJECT_INFO *obj)
{
    obj->control = Switch_Control;
    obj->collision = Switch_CollisionUW;
    obj->save_anim = 1;
    obj->save_flags = 1;
    obj->bounds = Switch_BoundsUW;
}

void Switch_Control(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];
    item->flags |= IF_CODE_BITS;
    if (!Item_IsTriggerActive(item)) {
        item->goal_anim_state = SWITCH_STATE_ON;
        item->timer = 0;
    }
    Item_Animate(item);
}

void Switch_Collision(int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll)
{
    if (g_Config.walk_to_items) {
        Switch_CollisionControlled(item_num, lara_item, coll);
        return;
    }

    ITEM_INFO *item = &g_Items[item_num];
    const OBJECT_INFO *const obj = &g_Objects[item->object_number];

    if (!g_Input.action || item->status != IS_NOT_ACTIVE
        || g_Lara.gun_status != LGS_ARMLESS || lara_item->gravity_status) {
        return;
    }

    if (lara_item->current_anim_state != LS_STOP) {
        return;
    }

    if (!Lara_TestPosition(item, obj->bounds())) {
        return;
    }

    lara_item->rot.y = item->rot.y;
    if (item->current_anim_state == SWITCH_STATE_ON) {
        Lara_AnimateUntil(lara_item, LS_SWITCH_ON);
        lara_item->goal_anim_state = LS_STOP;
        g_Lara.gun_status = LGS_HANDS_BUSY;
        item->status = IS_ACTIVE;
        item->goal_anim_state = SWITCH_STATE_OFF;
        Item_AddActive(item_num);
        Item_Animate(item);
    } else if (item->current_anim_state == SWITCH_STATE_OFF) {
        Lara_AnimateUntil(lara_item, LS_SWITCH_OFF);
        lara_item->goal_anim_state = LS_STOP;
        g_Lara.gun_status = LGS_HANDS_BUSY;
        item->status = IS_ACTIVE;
        item->goal_anim_state = SWITCH_STATE_ON;
        Item_AddActive(item_num);
        Item_Animate(item);
    }
}

void Switch_CollisionControlled(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll)
{
    ITEM_INFO *item = &g_Items[item_num];

    if ((g_Input.action && g_Lara.gun_status == LGS_ARMLESS
         && !lara_item->gravity_status
         && lara_item->current_anim_state == LS_STOP
         && item->status == IS_NOT_ACTIVE)
        || (g_Lara.interact_target.is_moving
            && g_Lara.interact_target.item_num == item_num)) {
        const BOUNDS_16 *bounds = Item_GetBoundsAccurate(item);

        m_Switch_BoundsControlled.shift.min.x = bounds->min.x - 256;
        m_Switch_BoundsControlled.shift.max.x = bounds->max.x + 256;
        m_Switch_BoundsControlled.shift.min.z = bounds->min.z - 200;
        m_Switch_BoundsControlled.shift.max.z = bounds->max.z + 200;

        XYZ_32 move_vector = { 0, 0, bounds->min.z - 64 };

        if (Lara_TestPosition(item, &m_Switch_BoundsControlled)) {
            if (Lara_MovePosition(item, &move_vector)) {
                if (item->current_anim_state == SWITCH_STATE_ON) {
                    Item_SwitchToAnim(lara_item, LA_WALL_SWITCH_DOWN, 0);
                    lara_item->current_anim_state = LS_SWITCH_OFF;
                    item->goal_anim_state = SWITCH_STATE_OFF;
                } else {
                    Item_SwitchToAnim(lara_item, LA_WALL_SWITCH_UP, 0);
                    lara_item->current_anim_state = LS_SWITCH_ON;
                    item->goal_anim_state = SWITCH_STATE_ON;
                }
                g_Lara.head_rot.x = 0;
                g_Lara.head_rot.y = 0;
                g_Lara.torso_rot.x = 0;
                g_Lara.torso_rot.y = 0;
                g_Lara.interact_target.is_moving = false;
                g_Lara.gun_status = LGS_HANDS_BUSY;
                Item_AddActive(item_num);
                item->status = IS_ACTIVE;
                Item_Animate(item);
            } else {
                g_Lara.interact_target.item_num = item_num;
            }
        } else if (
            g_Lara.interact_target.is_moving
            && g_Lara.interact_target.item_num == item_num) {
            g_Lara.interact_target.is_moving = false;
            g_Lara.gun_status = LGS_ARMLESS;
        }
    } else if (
        lara_item->current_anim_state != LS_SWITCH_ON
        && lara_item->current_anim_state != LS_SWITCH_OFF) {
        Object_Collision(item_num, lara_item, coll);
    }
}

void Switch_CollisionUW(int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll)
{
    ITEM_INFO *item = &g_Items[item_num];
    const OBJECT_INFO *const obj = &g_Objects[item->object_number];

    if (!g_Input.action || item->status != IS_NOT_ACTIVE
        || g_Lara.water_status != LWS_UNDERWATER) {
        return;
    }

    if (lara_item->current_anim_state != LS_TREAD) {
        return;
    }

    if (!Lara_TestPosition(item, obj->bounds())) {
        return;
    }

    if (item->current_anim_state == SWITCH_STATE_ON
        || item->current_anim_state == SWITCH_STATE_OFF) {
        XYZ_32 move_vector_uw = { 0, 0, 108 };
        if (!Lara_MovePosition(item, &move_vector_uw)) {
            return;
        }
        lara_item->fall_speed = 0;
        Lara_AnimateUntil(lara_item, LS_SWITCH_ON);
        lara_item->goal_anim_state = LS_TREAD;
        g_Lara.gun_status = LGS_HANDS_BUSY;
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

bool Switch_Trigger(int16_t item_num, int16_t timer)
{
    ITEM_INFO *item = &g_Items[item_num];
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
        item->status = IS_NOT_ACTIVE;
    }
    return true;
}
