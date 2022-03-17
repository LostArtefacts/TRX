#include "game/objects/switch.h"

#include "config.h"
#include "game/collide.h"
#include "game/control.h"
#include "game/draw.h"
#include "game/input.h"
#include "game/items.h"
#include "game/lara.h"
#include "global/vars.h"

static int16_t m_Switch_Bounds1[12] = {
    -200,
    +200,
    +0,
    +0,
    +WALL_L / 2 - 200,
    +WALL_L / 2,
    -10 * PHD_DEGREE,
    +10 * PHD_DEGREE,
    -30 * PHD_DEGREE,
    +30 * PHD_DEGREE,
    -10 * PHD_DEGREE,
    +10 * PHD_DEGREE,
};

static int16_t m_Switch_BoundsControlled[12] = {
    +0,
    +0,
    +0,
    +0,
    +0,
    +0,
    -10 * PHD_DEGREE,
    +10 * PHD_DEGREE,
    -30 * PHD_DEGREE,
    +30 * PHD_DEGREE,
    -10 * PHD_DEGREE,
    +10 * PHD_DEGREE,
};

static int16_t m_Switch_Bounds2[12] = {
    -WALL_L,          +WALL_L,          -WALL_L,          +WALL_L,
    -WALL_L,          +WALL_L / 2,      -80 * PHD_DEGREE, +80 * PHD_DEGREE,
    -80 * PHD_DEGREE, +80 * PHD_DEGREE, -80 * PHD_DEGREE, +80 * PHD_DEGREE,
};

void Switch_Setup1(OBJECT_INFO *obj)
{
    obj->control = Switch_Control;
    obj->collision = Switch_Collision;
    obj->save_anim = 1;
    obj->save_flags = 1;
}

void Switch_Setup2(OBJECT_INFO *obj)
{
    obj->control = Switch_Control;
    obj->collision = Switch_Collision2;
    obj->save_anim = 1;
    obj->save_flags = 1;
}

void Switch_Control(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];
    item->flags |= IF_CODE_BITS;
    if (!TriggerActive(item)) {
        item->goal_anim_state = SWITCH_STATE_ON;
        item->timer = 0;
    }
    AnimateItem(item);
}

void Switch_Collision(int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll)
{
    if (g_Config.walk_to_items) {
        Switch_CollisionControlled(item_num, lara_item, coll);
        return;
    }

    ITEM_INFO *item = &g_Items[item_num];

    if (!g_Input.action || item->status != IS_NOT_ACTIVE
        || g_Lara.gun_status != LGS_ARMLESS || lara_item->gravity_status) {
        return;
    }

    if (lara_item->current_anim_state != AS_STOP) {
        return;
    }

    if (!TestLaraPosition(m_Switch_Bounds1, item, lara_item)) {
        return;
    }

    lara_item->pos.y_rot = item->pos.y_rot;
    if (item->current_anim_state == SWITCH_STATE_ON) {
        AnimateLaraUntil(lara_item, AS_SWITCHON);
        lara_item->goal_anim_state = AS_STOP;
        g_Lara.gun_status = LGS_HANDSBUSY;
        item->status = IS_ACTIVE;
        item->goal_anim_state = SWITCH_STATE_OFF;
        AddActiveItem(item_num);
        AnimateItem(item);
    } else if (item->current_anim_state == SWITCH_STATE_OFF) {
        AnimateLaraUntil(lara_item, AS_SWITCHOFF);
        lara_item->goal_anim_state = AS_STOP;
        g_Lara.gun_status = LGS_HANDSBUSY;
        item->status = IS_ACTIVE;
        item->goal_anim_state = SWITCH_STATE_ON;
        AddActiveItem(item_num);
        AnimateItem(item);
    }
}

void Switch_CollisionControlled(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll)
{
    ITEM_INFO *item = &g_Items[item_num];

    if ((g_Input.action && g_Lara.gun_status == LGS_ARMLESS
         && !lara_item->gravity_status
         && lara_item->current_anim_state == AS_STOP
         && item->status == IS_NOT_ACTIVE)
        || (g_Lara.interact_target.is_moving
            && g_Lara.interact_target.item_num == item_num)) {
        int16_t *bounds = GetBoundsAccurate(item);

        m_Switch_BoundsControlled[0] = bounds[0] - 256;
        m_Switch_BoundsControlled[1] = bounds[1] + 256;
        m_Switch_BoundsControlled[4] = bounds[4] - 200;
        m_Switch_BoundsControlled[5] = bounds[5] + 200;

        PHD_VECTOR move_vector = { 0, 0, bounds[4] - 64 };

        if (TestLaraPosition(m_Switch_BoundsControlled, item, lara_item)) {
            if (MoveLaraPosition(&move_vector, item, lara_item)) {
                if (item->current_anim_state == SWITCH_STATE_ON) {
                    lara_item->anim_number = AA_WALLSWITCH_DOWN;
                    lara_item->current_anim_state = AS_SWITCHOFF;
                    item->goal_anim_state = SWITCH_STATE_OFF;
                } else {
                    lara_item->anim_number = AA_WALLSWITCH_UP;
                    lara_item->current_anim_state = AS_SWITCHON;
                    item->goal_anim_state = SWITCH_STATE_ON;
                }
                g_Lara.head_x_rot = 0;
                g_Lara.head_y_rot = 0;
                g_Lara.torso_x_rot = 0;
                g_Lara.torso_y_rot = 0;
                lara_item->frame_number =
                    g_Anims[lara_item->anim_number].frame_base;
                g_Lara.interact_target.is_moving = false;
                g_Lara.gun_status = LGS_HANDSBUSY;
                AddActiveItem(item_num);
                item->status = IS_ACTIVE;
                AnimateItem(item);
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
        lara_item->current_anim_state != AS_SWITCHON
        && lara_item->current_anim_state != AS_SWITCHOFF) {
        ObjectCollision(item_num, lara_item, coll);
    }
}

void Switch_Collision2(int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll)
{
    ITEM_INFO *item = &g_Items[item_num];

    if (!g_Input.action || item->status != IS_NOT_ACTIVE
        || g_Lara.water_status != LWS_UNDERWATER) {
        return;
    }

    if (lara_item->current_anim_state != AS_TREAD) {
        return;
    }

    if (!TestLaraPosition(m_Switch_Bounds2, item, lara_item)) {
        return;
    }

    if (item->current_anim_state == SWITCH_STATE_ON
        || item->current_anim_state == SWITCH_STATE_OFF) {
        PHD_VECTOR move_vector_uw = { 0, 0, 108 };
        if (!MoveLaraPosition(&move_vector_uw, item, lara_item)) {
            return;
        }
        lara_item->fall_speed = 0;
        AnimateLaraUntil(lara_item, AS_SWITCHON);
        lara_item->goal_anim_state = AS_TREAD;
        g_Lara.gun_status = LGS_HANDSBUSY;
        item->status = IS_ACTIVE;
        if (item->current_anim_state == SWITCH_STATE_ON) {
            item->goal_anim_state = SWITCH_STATE_OFF;
        } else {
            item->goal_anim_state = SWITCH_STATE_ON;
        }
        AddActiveItem(item_num);
        AnimateItem(item);
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
            item->timer *= FRAMES_PER_SECOND;
        }
        item->status = IS_ACTIVE;
    } else {
        RemoveActiveItem(item_num);
        item->status = IS_NOT_ACTIVE;
    }
    return true;
}
