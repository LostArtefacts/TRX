#include "game/effects.h"
#include "game/input.h"
#include "game/lara/common.h"
#include "game/random.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/game/collision.h>
#include <libtrx/game/lara.h>
#include <libtrx/game/math.h>
#include <libtrx/utils.h>

bool Lara_TestVault(ITEM *item, COLL_INFO *coll)
{
    if (coll->coll_type != COLL_FRONT || !g_Input.action
        || g_Lara.gun_status != LGS_ARMLESS
        || ABS(coll->side_left.floor - coll->side_right.floor) >= SLOPE_DIF) {
        return false;
    }

    PHD_ANGLE angle = item->rot.y;
    if (angle >= 0 - LARA_VAULT_ANGLE && angle <= 0 + LARA_VAULT_ANGLE) {
        angle = 0;
    } else if (
        angle >= DEG_90 - LARA_VAULT_ANGLE
        && angle <= DEG_90 + LARA_VAULT_ANGLE) {
        angle = DEG_90;
    } else if (
        angle >= (DEG_180 - 1) - LARA_VAULT_ANGLE
        || angle <= -(DEG_180 - 1) + LARA_VAULT_ANGLE) {
        angle = -DEG_180;
    } else if (
        angle >= -DEG_90 - LARA_VAULT_ANGLE
        && angle <= -DEG_90 + LARA_VAULT_ANGLE) {
        angle = -DEG_90;
    }

    if (angle & (DEG_90 - 1)) {
        return false;
    }

    int32_t hdif = coll->side_front.floor;
    if (hdif >= -STEP_L * 2 - STEP_L / 2 && hdif <= -STEP_L * 2 + STEP_L / 2) {
        if (hdif - coll->side_front.ceiling < 0
            || coll->side_left.floor - coll->side_left.ceiling < 0
            || coll->side_right.floor - coll->side_right.ceiling < 0) {
            return false;
        }
        item->current_anim_state = LS_PULL_UP;
        item->goal_anim_state = LS_STOP;
        Item_SwitchToAnim(item, LA_CLIMB_2CLICK, 0);
        item->pos.y += STEP_L * 2 + hdif;
        g_Lara.gun_status = LGS_HANDS_BUSY;
        item->rot.y = angle;
        Lara_ShiftCol(coll);
        return true;
    } else if (
        hdif >= -STEP_L * 3 - STEP_L / 2 && hdif <= -STEP_L * 3 + STEP_L / 2) {
        if (hdif - coll->side_front.ceiling < 0
            || coll->side_left.floor - coll->side_left.ceiling < 0
            || coll->side_right.floor - coll->side_right.ceiling < 0) {
            return false;
        }
        item->current_anim_state = LS_PULL_UP;
        item->goal_anim_state = LS_STOP;
        Item_SwitchToAnim(item, LA_CLIMB_3CLICK, 0);
        item->pos.y += STEP_L * 3 + hdif;
        g_Lara.gun_status = LGS_HANDS_BUSY;
        item->rot.y = angle;
        Lara_ShiftCol(coll);
        return true;
    } else if (
        hdif >= -STEP_L * 7 - STEP_L / 2 && hdif <= -STEP_L * 4 + STEP_L / 2) {
        item->goal_anim_state = LS_JUMP_UP;
        item->current_anim_state = LS_STOP;
        Item_SwitchToAnim(item, LA_STAND_STILL, 0);
        g_Lara.calc_fall_speed =
            -(int16_t)(Math_Sqrt((int)(-2 * GRAVITY * (hdif + 800))) + 3);
        Lara_Animate(item);
        item->rot.y = angle;
        Lara_ShiftCol(coll);
        return true;
    }

    return false;
}

void Lara_CatchFire(void)
{
    const int16_t effect_num = Effect_Create(g_LaraItem->room_num);
    if (effect_num != NO_EFFECT) {
        EFFECT *const effect = Effect_Get(effect_num);
        effect->frame_num = 0;
        effect->object_id = O_FLAME;
        effect->counter = -1;
    }
}

void Lara_Extinguish(void)
{
    // put out flame objects
    int16_t effect_num = Effect_GetActiveNum();
    while (effect_num != NO_EFFECT) {
        EFFECT *const effect = Effect_Get(effect_num);
        const int16_t next_effect_num = effect->next_active;
        if (effect->object_id == O_FLAME && effect->counter < 0) {
            effect->counter = 0;
            Effect_Kill(effect_num);
        }
        effect_num = next_effect_num;
    }
}
