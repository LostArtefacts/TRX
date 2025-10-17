#include "game/lara.h"
#include "game/objects/common.h"

#include <libtrx/config.h>
#include <libtrx/game/input.h>
#include <libtrx/game/objects/general/switch.h>

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

        OBJECT_BOUNDS col_bounds = *Object_Get(item->object_id)->bounds_func();
        col_bounds.shift.min.x = bounds->min.x - 256;
        col_bounds.shift.max.x = bounds->max.x + 256;
        col_bounds.shift.min.z = bounds->min.z - 200;
        col_bounds.shift.max.z = bounds->max.z + 200;

        XYZ_32 move_vector = { 0, 0, bounds->min.z - 64 };

        if (Lara_TestPosition(item, &col_bounds)) {
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

void Switch_Collision(
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

void Switch_CollisionUW(
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
