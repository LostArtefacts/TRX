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

#define LF_FASTFALL 1

void Lara_SlideSlope(ITEM *item, COLL_INFO *coll)
{
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -512;
    coll->bad_ceiling = 0;
    Lara_GetCollisionInfo(item, coll);

    if (Lara_HitCeiling(item, coll)) {
        return;
    }

    Lara_DeflectEdge(item, coll);

    if (coll->side_mid.floor > 200) {
        if (item->current_anim_state == LS_SLIDE) {
            item->current_anim_state = LS_JUMP_FORWARD;
            item->goal_anim_state = LS_JUMP_FORWARD;
            Item_SwitchToAnim(item, LA_FALL_START, 0);
        } else {
            item->current_anim_state = LS_FALL_BACK;
            item->goal_anim_state = LS_FALL_BACK;
            Item_SwitchToAnim(item, LA_FALL_BACK, 0);
        }
        item->gravity = true;
        item->fall_speed = 0;
        return;
    }

    Lara_TestSlide(item, coll);
    item->pos.y += coll->side_mid.floor;

    if (ABS(coll->tilt_x) <= 2 && ABS(coll->tilt_z) <= 2) {
        item->goal_anim_state = LS_STOP;
    }
}

bool Lara_HitCeiling(ITEM *item, COLL_INFO *coll)
{
    if (coll->coll_type != COLL_TOP && coll->coll_type != COLL_CLAMP) {
        return false;
    }

    item->pos.x = coll->old.x;
    item->pos.y = coll->old.y;
    item->pos.z = coll->old.z;
    item->goal_anim_state = LS_STOP;
    item->current_anim_state = LS_STOP;
    Item_SwitchToAnim(item, LA_STAND_STILL, 0);
    item->gravity = false;
    item->fall_speed = 0;
    item->speed = 0;
    return true;
}
bool Lara_DeflectEdge(ITEM *item, COLL_INFO *coll)
{
    if (coll->coll_type == COLL_FRONT || coll->coll_type == COLL_TOP_FRONT) {
        Lara_ShiftCol(coll);
        item->goal_anim_state = LS_STOP;
        item->current_anim_state = LS_STOP;
        item->gravity = false;
        item->speed = 0;
        return true;
    }

    if (coll->coll_type == COLL_LEFT) {
        Lara_ShiftCol(coll);
        item->rot.y += LARA_DEFLECT_ANGLE;
    } else if (coll->coll_type == COLL_RIGHT) {
        Lara_ShiftCol(coll);
        item->rot.y -= LARA_DEFLECT_ANGLE;
    }
    return false;
}

void Lara_DeflectEdgeJump(ITEM *item, COLL_INFO *coll)
{
    Lara_ShiftCol(coll);
    switch (coll->coll_type) {
    case COLL_LEFT:
        item->rot.y += LARA_DEFLECT_ANGLE;
        break;

    case COLL_RIGHT:
        item->rot.y -= LARA_DEFLECT_ANGLE;
        break;

    case COLL_FRONT:
    case COLL_TOP_FRONT:
        item->goal_anim_state = LS_FAST_FALL;
        item->current_anim_state = LS_FAST_FALL;
        Item_SwitchToAnim(item, LA_SMASH_JUMP, LF_FASTFALL);
        item->speed /= 4;
        g_Lara.move_angle -= DEG_180;
        if (item->fall_speed <= 0) {
            item->fall_speed = 1;
        }
        break;

    case COLL_TOP:
        if (item->fall_speed <= 0) {
            item->fall_speed = 1;
        }
        break;

    case COLL_CLAMP:
        item->pos.z -= (Math_Cos(coll->facing) * 100) >> W2V_SHIFT;
        item->pos.x -= (Math_Sin(coll->facing) * 100) >> W2V_SHIFT;
        item->speed = 0;
        coll->side_mid.floor = 0;
        if (item->fall_speed <= 0) {
            item->fall_speed = 16;
        }
        break;
    }
}

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

bool Lara_TestSlide(ITEM *item, COLL_INFO *coll)
{
    static PHD_ANGLE old_angle = 1;

    if (ABS(coll->tilt_x) <= 2 && ABS(coll->tilt_z) <= 2) {
        return false;
    }

    PHD_ANGLE ang = 0;
    if (coll->tilt_x > 2) {
        ang = -DEG_90;
    } else if (coll->tilt_x < -2) {
        ang = DEG_90;
    }
    if (coll->tilt_z > 2 && coll->tilt_z > ABS(coll->tilt_x)) {
        ang = -DEG_180;
    } else if (coll->tilt_z < -2 && -coll->tilt_z > ABS(coll->tilt_x)) {
        ang = 0;
    }

    PHD_ANGLE adif = ang - item->rot.y;
    Lara_ShiftCol(coll);
    if (adif >= -DEG_90 && adif <= DEG_90) {
        if (item->current_anim_state != LS_SLIDE || old_angle != ang) {
            item->goal_anim_state = LS_SLIDE;
            item->current_anim_state = LS_SLIDE;
            Item_SwitchToAnim(item, LA_SLIDE_FORWARD, 0);
            item->rot.y = ang;
            g_Lara.move_angle = ang;
            old_angle = ang;
        }
    } else {
        if (item->current_anim_state != LS_SLIDE_BACK || old_angle != ang) {
            item->goal_anim_state = LS_SLIDE_BACK;
            item->current_anim_state = LS_SLIDE_BACK;
            Item_SwitchToAnim(item, LA_SLIDE_BACKWARD_START, 0);
            item->rot.y = ang - DEG_180;
            g_Lara.move_angle = ang;
            old_angle = ang;
        }
    }
    return true;
}

bool Lara_LandedBad(ITEM *item, COLL_INFO *coll)
{
    int16_t room_num = item->room_num;

    const SECTOR *const sector =
        Room_GetSector(item->pos.x, item->pos.y, item->pos.z, &room_num);

    const int32_t old_y = item->pos.y;
    const int32_t height = Room_GetHeight(
        sector, item->pos.x, item->pos.y - LARA_HEIGHT, item->pos.z);

    item->floor = height;
    item->pos.y = height;
    Room_TestTriggers(item);
    item->pos.y = old_y;

    int landspeed = item->fall_speed - DAMAGE_START;
    if (landspeed <= 0) {
        return false;
    } else if (landspeed > DAMAGE_LENGTH) {
        item->hit_points = -1;
    } else {
        Lara_TakeDamage(
            (LARA_MAX_HITPOINTS * landspeed * landspeed)
                / (DAMAGE_LENGTH * DAMAGE_LENGTH),
            false);
    }

    // #675: Original bug to keep. Correct operator would be <=
    if (item->hit_points < 0) {
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
