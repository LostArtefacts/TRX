#include "game/lara/lara_misc.h"

#include "config.h"
#include "game/collide.h"
#include "game/input.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/room.h"
#include "global/const.h"
#include "global/vars.h"
#include "math/math.h"
#include "util.h"

#include <stdint.h>

#define LF_FASTFALL 1
#define LF_STOPHANG 9
#define LF_STARTHANG 12
#define LF_HANG 21

void Lara_GetCollisionInfo(ITEM_INFO *item, COLL_INFO *coll)
{
    coll->facing = g_Lara.move_angle;
    Collide_GetCollisionInfo(
        coll, item->pos.x, item->pos.y, item->pos.z, item->room_number,
        LARA_HEIGHT);
}

void Lara_HangTest(ITEM_INFO *item, COLL_INFO *coll)
{
    int flag = 0;
    int16_t *bounds;

    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = NO_BAD_NEG;
    coll->bad_ceiling = 0;
    Lara_GetCollisionInfo(item, coll);
    if (coll->front_floor < 200) {
        flag = 1;
    }

    g_Lara.move_angle = item->rot.y;
    item->gravity_status = 0;
    item->fall_speed = 0;

    PHD_ANGLE angle = (uint16_t)(item->rot.y + PHD_45) / PHD_90;
    switch (angle) {
    case DIR_NORTH:
        item->pos.z += 2;
        break;

    case DIR_WEST:
        item->pos.x -= 2;
        break;

    case DIR_SOUTH:
        item->pos.z -= 2;
        break;

    case DIR_EAST:
        item->pos.x += 2;
        break;
    }

    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = 0;
    Lara_GetCollisionInfo(item, coll);

    if (!g_Input.action || item->hit_points <= 0) {
        item->goal_anim_state = LS_JUMP_UP;
        item->current_anim_state = LS_JUMP_UP;
        Item_SwitchToAnim(item, LA_STOP_HANG, LF_STOPHANG);
        bounds = Item_GetBoundsAccurate(item);
        if (g_Config.enable_swing_cancel && item->hit_points > 0) {
            item->pos.y += bounds[FRAME_BOUND_MAX_Y];
        } else {
            item->pos.y += coll->front_floor - bounds[FRAME_BOUND_MIN_Y] + 2;
        }
        item->pos.x += coll->shift.x;
        item->pos.z += coll->shift.z;
        item->gravity_status = 1;
        item->fall_speed = 1;
        item->speed = 2;
        g_Lara.gun_status = LGS_ARMLESS;
        return;
    }

    bounds = Item_GetBoundsAccurate(item);
    int32_t hdif = coll->front_floor - bounds[FRAME_BOUND_MIN_Y];

    if (ABS(coll->left_floor - coll->right_floor) >= SLOPE_DIF
        || coll->mid_ceiling >= 0 || coll->coll_type != COLL_FRONT
        || hdif < -SLOPE_DIF || hdif > SLOPE_DIF || flag) {
        item->pos.x = coll->old.x;
        item->pos.y = coll->old.y;
        item->pos.z = coll->old.z;
        if (item->current_anim_state == LS_HANG_LEFT
            || item->current_anim_state == LS_HANG_RIGHT) {
            item->goal_anim_state = LS_HANG;
            item->current_anim_state = LS_HANG;
            Item_SwitchToAnim(item, LA_HANG, LF_HANG);
        }
        return;
    }

    switch (angle) {
    case DIR_NORTH:
    case DIR_SOUTH:
        item->pos.z += coll->shift.z;
        break;

    case DIR_WEST:
    case DIR_EAST:
        item->pos.x += coll->shift.x;
        break;
    }

    if (hdif >= -STEP_L && hdif <= STEP_L) {
        item->pos.y += hdif;
    }
}

void Lara_SlideSlope(ITEM_INFO *item, COLL_INFO *coll)
{
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -512;
    coll->bad_ceiling = 0;
    Lara_GetCollisionInfo(item, coll);

    if (Lara_HitCeiling(item, coll)) {
        return;
    }

    Lara_DeflectEdge(item, coll);

    if (coll->mid_floor > 200) {
        if (item->current_anim_state == LS_SLIDE) {
            item->current_anim_state = LS_JUMP_FORWARD;
            item->goal_anim_state = LS_JUMP_FORWARD;
            Item_SwitchToAnim(item, LA_FALL_DOWN, 0);
        } else {
            item->current_anim_state = LS_FALL_BACK;
            item->goal_anim_state = LS_FALL_BACK;
            Item_SwitchToAnim(item, LA_FALL_BACK, 0);
        }
        item->gravity_status = 1;
        item->fall_speed = 0;
        return;
    }

    Lara_TestSlide(item, coll);
    item->pos.y += coll->mid_floor;

    if (ABS(coll->tilt_x) <= 2 && ABS(coll->tilt_z) <= 2) {
        item->goal_anim_state = LS_STOP;
    }
}

bool Lara_Fallen(ITEM_INFO *item, COLL_INFO *coll)
{
    if (coll->mid_floor <= STEPUP_HEIGHT) {
        return false;
    }
    item->current_anim_state = LS_JUMP_FORWARD;
    item->goal_anim_state = LS_JUMP_FORWARD;
    Item_SwitchToAnim(item, LA_FALL_DOWN, 0);
    item->gravity_status = 1;
    item->fall_speed = 0;
    return true;
}

bool Lara_HitCeiling(ITEM_INFO *item, COLL_INFO *coll)
{
    if (coll->coll_type != COLL_TOP && coll->coll_type != COLL_CLAMP) {
        return false;
    }

    item->pos.x = coll->old.x;
    item->pos.y = coll->old.y;
    item->pos.z = coll->old.z;
    item->goal_anim_state = LS_STOP;
    item->current_anim_state = LS_STOP;
    Item_SwitchToAnim(item, LA_STOP, 0);
    item->gravity_status = 0;
    item->fall_speed = 0;
    item->speed = 0;
    return true;
}
bool Lara_DeflectEdge(ITEM_INFO *item, COLL_INFO *coll)
{
    if (coll->coll_type == COLL_FRONT || coll->coll_type == COLL_TOPFRONT) {
        Item_ShiftCol(item, coll);
        item->goal_anim_state = LS_STOP;
        item->current_anim_state = LS_STOP;
        item->gravity_status = 0;
        item->speed = 0;
        return true;
    }

    if (coll->coll_type == COLL_LEFT) {
        Item_ShiftCol(item, coll);
        item->rot.y += LARA_DEF_ADD_EDGE;
    } else if (coll->coll_type == COLL_RIGHT) {
        Item_ShiftCol(item, coll);
        item->rot.y -= LARA_DEF_ADD_EDGE;
    }
    return false;
}

void Lara_DeflectEdgeJump(ITEM_INFO *item, COLL_INFO *coll)
{
    Item_ShiftCol(item, coll);
    switch (coll->coll_type) {
    case COLL_LEFT:
        item->rot.y += LARA_DEF_ADD_EDGE;
        break;

    case COLL_RIGHT:
        item->rot.y -= LARA_DEF_ADD_EDGE;
        break;

    case COLL_FRONT:
    case COLL_TOPFRONT:
        item->goal_anim_state = LS_FAST_FALL;
        item->current_anim_state = LS_FAST_FALL;
        Item_SwitchToAnim(item, LA_FAST_FALL, LF_FASTFALL);
        item->speed /= 4;
        g_Lara.move_angle -= PHD_180;
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
        coll->mid_floor = 0;
        if (item->fall_speed <= 0) {
            item->fall_speed = 16;
        }
        break;
    }
}

void Lara_SlideEdgeJump(ITEM_INFO *item, COLL_INFO *coll)
{
    Item_ShiftCol(item, coll);
    switch (coll->coll_type) {
    case COLL_LEFT:
        item->rot.y += LARA_DEF_ADD_EDGE;
        break;

    case COLL_RIGHT:
        item->rot.y -= LARA_DEF_ADD_EDGE;
        break;

    case COLL_TOP:
    case COLL_TOPFRONT:
        if (item->fall_speed <= 0) {
            item->fall_speed = 1;
        }
        break;

    case COLL_CLAMP:
        item->pos.z -= (Math_Cos(coll->facing) * 100) >> W2V_SHIFT;
        item->pos.x -= (Math_Sin(coll->facing) * 100) >> W2V_SHIFT;
        item->speed = 0;
        coll->mid_floor = 0;
        if (item->fall_speed <= 0) {
            item->fall_speed = 16;
        }
        break;
    }
}

bool Lara_TestVault(ITEM_INFO *item, COLL_INFO *coll)
{
    if (coll->coll_type != COLL_FRONT || !g_Input.action
        || g_Lara.gun_status != LGS_ARMLESS
        || ABS(coll->left_floor - coll->right_floor) >= SLOPE_DIF) {
        return false;
    }

    PHD_ANGLE angle = item->rot.y;
    if (angle >= 0 - VAULT_ANGLE && angle <= 0 + VAULT_ANGLE) {
        angle = 0;
    } else if (angle >= PHD_90 - VAULT_ANGLE && angle <= PHD_90 + VAULT_ANGLE) {
        angle = PHD_90;
    } else if (
        angle >= (PHD_180 - 1) - VAULT_ANGLE
        || angle <= -(PHD_180 - 1) + VAULT_ANGLE) {
        angle = -PHD_180;
    } else if (
        angle >= -PHD_90 - VAULT_ANGLE && angle <= -PHD_90 + VAULT_ANGLE) {
        angle = -PHD_90;
    }

    if (angle & (PHD_90 - 1)) {
        return false;
    }

    int32_t hdif = coll->front_floor;
    if (hdif >= -STEP_L * 2 - STEP_L / 2 && hdif <= -STEP_L * 2 + STEP_L / 2) {
        if (hdif - coll->front_ceiling < 0
            || coll->left_floor - coll->left_ceiling < 0
            || coll->right_floor - coll->right_ceiling < 0) {
            return false;
        }
        item->current_anim_state = LS_NULL;
        item->goal_anim_state = LS_STOP;
        Item_SwitchToAnim(item, LA_VAULT_12, 0);
        item->pos.y += STEP_L * 2 + hdif;
        g_Lara.gun_status = LGS_HANDS_BUSY;
        item->rot.y = angle;
        Item_ShiftCol(item, coll);
        return true;
    } else if (
        hdif >= -STEP_L * 3 - STEP_L / 2 && hdif <= -STEP_L * 3 + STEP_L / 2) {
        if (hdif - coll->front_ceiling < 0
            || coll->left_floor - coll->left_ceiling < 0
            || coll->right_floor - coll->right_ceiling < 0) {
            return false;
        }
        item->current_anim_state = LS_NULL;
        item->goal_anim_state = LS_STOP;
        Item_SwitchToAnim(item, LA_VAULT_34, 0);
        item->pos.y += STEP_L * 3 + hdif;
        g_Lara.gun_status = LGS_HANDS_BUSY;
        item->rot.y = angle;
        Item_ShiftCol(item, coll);
        return true;
    } else if (
        hdif >= -STEP_L * 7 - STEP_L / 2 && hdif <= -STEP_L * 4 + STEP_L / 2) {
        item->goal_anim_state = LS_JUMP_UP;
        item->current_anim_state = LS_STOP;
        Item_SwitchToAnim(item, LA_STOP, 0);
        g_Lara.calc_fall_speed =
            -(int16_t)(Math_Sqrt((int)(-2 * GRAVITY * (hdif + 800))) + 3);
        Lara_Animate(item);
        item->rot.y = angle;
        Item_ShiftCol(item, coll);
        return true;
    }

    return false;
}

bool Lara_TestHangJump(ITEM_INFO *item, COLL_INFO *coll)
{
    int hdif;
    int16_t *bounds;

    if (coll->coll_type != COLL_FRONT || !g_Input.action
        || g_Lara.gun_status != LGS_ARMLESS
        || ABS(coll->left_floor - coll->right_floor) >= SLOPE_DIF) {
        return false;
    }

    if (coll->front_ceiling > 0 || coll->mid_ceiling > -384
        || coll->mid_floor < 200) {
        return false;
    }

    bounds = Item_GetBoundsAccurate(item);
    hdif = coll->front_floor - bounds[FRAME_BOUND_MIN_Y];
    if (hdif < 0 && hdif + item->fall_speed < 0) {
        return false;
    }
    if (hdif > 0 && hdif + item->fall_speed > 0) {
        return false;
    }

    PHD_ANGLE angle = item->rot.y;
    if (angle >= -HANG_ANGLE && angle <= HANG_ANGLE) {
        angle = 0;
    } else if (angle >= PHD_90 - HANG_ANGLE && angle <= PHD_90 + HANG_ANGLE) {
        angle = PHD_90;
    } else if (
        angle >= (PHD_180 - 1) - HANG_ANGLE
        || angle <= -(PHD_180 - 1) + HANG_ANGLE) {
        angle = -PHD_180;
    } else if (angle >= -PHD_90 - HANG_ANGLE && angle <= -PHD_90 + HANG_ANGLE) {
        angle = -PHD_90;
    }

    if (angle & (PHD_90 - 1)) {
        return false;
    }

    if (Lara_TestHangSwingIn(item, angle)) {
        Item_SwitchToAnim(item, LA_GRAB_LEDGE_IN, 0);
    } else {
        Item_SwitchToAnim(item, LA_GRAB_LEDGE, 0);
    }
    item->current_anim_state = LS_HANG;
    item->goal_anim_state = LS_HANG;

    // bounds = Item_GetBoundsAccurate(item);
    item->pos.y += hdif;
    item->pos.x += coll->shift.x;
    item->pos.z += coll->shift.z;
    item->rot.y = angle;
    item->gravity_status = 0;
    item->fall_speed = 0;
    item->speed = 0;
    g_Lara.gun_status = LGS_HANDS_BUSY;
    return true;
}

bool Lara_TestHangSwingIn(ITEM_INFO *item, PHD_ANGLE angle)
{
    int x = item->pos.x;
    int y = item->pos.y;
    int z = item->pos.z;
    int16_t room_num = item->room_number;
    switch (angle) {
    case 0:
        z += 256;
        break;
    case PHD_90:
        x += 256;
        break;
    case -PHD_90:
        x -= 256;
        break;
    case -PHD_180:
        z -= 256;
        break;
    }

    FLOOR_INFO *floor = Room_GetFloor(x, y, z, &room_num);
    int h = Room_GetHeight(floor, x, y, z);
    int c = Room_GetCeiling(floor, x, y, z);

    if (h != NO_HEIGHT) {
        if ((h - y) > 0 && (c - y) < -400) {
            return true;
        }
    }
    return false;
}

bool Lara_TestHangJumpUp(ITEM_INFO *item, COLL_INFO *coll)
{
    int hdif;
    int16_t *bounds;

    if (coll->coll_type != COLL_FRONT || !g_Input.action
        || g_Lara.gun_status != LGS_ARMLESS
        || ABS(coll->left_floor - coll->right_floor) >= SLOPE_DIF) {
        return false;
    }

    if (coll->front_ceiling > 0 || coll->mid_ceiling > -384) {
        return false;
    }

    bounds = Item_GetBoundsAccurate(item);
    hdif = coll->front_floor - bounds[FRAME_BOUND_MIN_Y];
    if (hdif < 0 && hdif + item->fall_speed < 0) {
        return false;
    }
    if (hdif > 0 && hdif + item->fall_speed > 0) {
        return false;
    }

    PHD_ANGLE angle = item->rot.y;
    if (angle >= 0 - HANG_ANGLE && angle <= 0 + HANG_ANGLE) {
        angle = 0;
    } else if (angle >= PHD_90 - HANG_ANGLE && angle <= PHD_90 + HANG_ANGLE) {
        angle = PHD_90;
    } else if (
        angle >= (PHD_180 - 1) - HANG_ANGLE
        || angle <= -(PHD_180 - 1) + HANG_ANGLE) {
        angle = -PHD_180;
    } else if (angle >= -PHD_90 - HANG_ANGLE && angle <= -PHD_90 + HANG_ANGLE) {
        angle = -PHD_90;
    }

    if (angle & (PHD_90 - 1)) {
        return false;
    }

    item->goal_anim_state = LS_HANG;
    item->current_anim_state = LS_HANG;
    Item_SwitchToAnim(item, LA_HANG, LF_STARTHANG);
    bounds = Item_GetBoundsAccurate(item);
    item->pos.y += coll->front_floor - bounds[FRAME_BOUND_MIN_Y];
    item->pos.x += coll->shift.x;
    item->pos.z += coll->shift.z;
    item->rot.y = angle;
    item->gravity_status = 0;
    item->fall_speed = 0;
    item->speed = 0;
    g_Lara.gun_status = LGS_HANDS_BUSY;
    return true;
}

bool Lara_TestSlide(ITEM_INFO *item, COLL_INFO *coll)
{
    static PHD_ANGLE old_angle = 1;

    if (ABS(coll->tilt_x) <= 2 && ABS(coll->tilt_z) <= 2) {
        return false;
    }

    PHD_ANGLE ang = 0;
    if (coll->tilt_x > 2) {
        ang = -PHD_90;
    } else if (coll->tilt_x < -2) {
        ang = PHD_90;
    }
    if (coll->tilt_z > 2 && coll->tilt_z > ABS(coll->tilt_x)) {
        ang = -PHD_180;
    } else if (coll->tilt_z < -2 && -coll->tilt_z > ABS(coll->tilt_x)) {
        ang = 0;
    }

    PHD_ANGLE adif = ang - item->rot.y;
    Item_ShiftCol(item, coll);
    if (adif >= -PHD_90 && adif <= PHD_90) {
        if (item->current_anim_state != LS_SLIDE || old_angle != ang) {
            item->goal_anim_state = LS_SLIDE;
            item->current_anim_state = LS_SLIDE;
            Item_SwitchToAnim(item, LA_SLIDE, 0);
            item->rot.y = ang;
            g_Lara.move_angle = ang;
            old_angle = ang;
        }
    } else {
        if (item->current_anim_state != LS_SLIDE_BACK || old_angle != ang) {
            item->goal_anim_state = LS_SLIDE_BACK;
            item->current_anim_state = LS_SLIDE_BACK;
            Item_SwitchToAnim(item, LA_SLIDE_BACK, 0);
            item->rot.y = ang - PHD_180;
            g_Lara.move_angle = ang;
            old_angle = ang;
        }
    }
    return true;
}

bool Lara_LandedBad(ITEM_INFO *item, COLL_INFO *coll)
{
    int16_t room_num = item->room_number;

    FLOOR_INFO *floor =
        Room_GetFloor(item->pos.x, item->pos.y, item->pos.z, &room_num);

    int oy = item->pos.y;
    int height = Room_GetHeight(
        floor, item->pos.x, item->pos.y - LARA_HEIGHT, item->pos.z);

    item->floor = height;
    item->pos.y = height;
    Room_TestTriggers(g_TriggerIndex, false);
    item->pos.y = oy;

    int landspeed = item->fall_speed - DAMAGE_START;
    if (landspeed <= 0) {
        return false;
    } else if (landspeed > DAMAGE_LENGTH) {
        item->hit_points = -1;
    } else {
        Lara_TakeDamage(
            (LARA_HITPOINTS * landspeed * landspeed)
                / (DAMAGE_LENGTH * DAMAGE_LENGTH),
            false);
    }

    if (item->hit_points < 0) {
        return true;
    }
    return false;
}

void Lara_SurfaceCollision(ITEM_INFO *item, COLL_INFO *coll)
{
    coll->facing = g_Lara.move_angle;

    Collide_GetCollisionInfo(
        coll, item->pos.x, item->pos.y + SURF_HEIGHT, item->pos.z,
        item->room_number, SURF_HEIGHT);

    Item_ShiftCol(item, coll);

    if ((coll->coll_type
         & (COLL_FRONT | COLL_LEFT | COLL_RIGHT | COLL_TOP | COLL_TOPFRONT
            | COLL_CLAMP))
        || coll->mid_floor < 0) {
        item->fall_speed = 0;
        item->pos.x = coll->old.x;
        item->pos.y = coll->old.y;
        item->pos.z = coll->old.z;
    } else if (coll->coll_type == COLL_LEFT) {
        item->rot.y += 5 * PHD_DEGREE;
    } else if (coll->coll_type == COLL_RIGHT) {
        item->rot.y -= 5 * PHD_DEGREE;
    }

    int16_t wh = Room_GetWaterHeight(
        item->pos.x, item->pos.y, item->pos.z, item->room_number);
    if (wh - item->pos.y <= -100) {
        item->goal_anim_state = LS_SWIM;
        item->current_anim_state = LS_DIVE;
        Item_SwitchToAnim(item, LA_SURF_DIVE, 0);
        item->rot.x = -45 * PHD_DEGREE;
        item->fall_speed = 80;
        g_Lara.water_status = LWS_UNDERWATER;
        return;
    }

    Lara_TestWaterClimbOut(item, coll);
}

bool Lara_TestWaterClimbOut(ITEM_INFO *item, COLL_INFO *coll)
{
    if (item->rot.y != g_Lara.move_angle) {
        return false;
    }

    if (coll->coll_type != COLL_FRONT || !g_Input.action
        || ABS(coll->left_floor - coll->right_floor) >= SLOPE_DIF) {
        return false;
    }

    if (coll->front_ceiling > 0 || coll->mid_ceiling > -STEPUP_HEIGHT) {
        return false;
    }

    int hdif = coll->front_floor + 700;
    if (hdif < -512 || hdif > 100) {
        return false;
    }

    PHD_ANGLE angle = item->rot.y;
    if (angle >= -HANG_ANGLE && angle <= HANG_ANGLE) {
        angle = 0;
    } else if (angle >= PHD_90 - HANG_ANGLE && angle <= PHD_90 + HANG_ANGLE) {
        angle = PHD_90;
    } else if (
        angle >= (PHD_180 - 1) - HANG_ANGLE
        || angle <= -(PHD_180 - 1) + HANG_ANGLE) {
        angle = -PHD_180;
    } else if (angle >= -PHD_90 - HANG_ANGLE && angle <= -PHD_90 + HANG_ANGLE) {
        angle = -PHD_90;
    }
    if (angle & (PHD_90 - 1)) {
        return false;
    }

    item->pos.y += hdif - 5;
    Item_UpdateRoom(item, -LARA_HEIGHT / 2);

    switch (angle) {
    case 0:
        item->pos.z = (item->pos.z & -WALL_L) + WALL_L + LARA_RAD;
        break;
    case PHD_90:
        item->pos.x = (item->pos.x & -WALL_L) + WALL_L + LARA_RAD;
        break;
    case -PHD_180:
        item->pos.z = (item->pos.z & -WALL_L) - LARA_RAD;
        break;
    case -PHD_90:
        item->pos.x = (item->pos.x & -WALL_L) - LARA_RAD;
        break;
    }

    Item_SwitchToAnim(item, LA_SURF_CLIMB, 0);
    item->current_anim_state = LS_WATER_OUT;
    item->goal_anim_state = LS_STOP;
    item->rot.x = 0;
    item->rot.y = angle;
    item->rot.z = 0;
    item->gravity_status = 0;
    item->fall_speed = 0;
    item->speed = 0;
    g_Lara.gun_status = LGS_HANDS_BUSY;
    g_Lara.water_status = LWS_ABOVE_WATER;
    return true;
}

void Lara_SwimCollision(ITEM_INFO *item, COLL_INFO *coll)
{
    if (item->rot.x >= -PHD_90 && item->rot.x <= PHD_90) {
        g_Lara.move_angle = coll->facing = item->rot.y;
    } else {
        g_Lara.move_angle = coll->facing = item->rot.y - PHD_180;
    }
    Collide_GetCollisionInfo(
        coll, item->pos.x, item->pos.y + UW_HEIGHT / 2, item->pos.z,
        item->room_number, UW_HEIGHT);

    Item_ShiftCol(item, coll);

    switch (coll->coll_type) {
    case COLL_FRONT:
        if (item->rot.x > 35 * PHD_DEGREE) {
            item->rot.x = item->rot.x + UW_WALLDEFLECT;
            break;
        }
        if (item->rot.x < -35 * PHD_DEGREE) {
            item->rot.x = item->rot.x - UW_WALLDEFLECT;
            break;
        }
        item->fall_speed = 0;
        break;

    case COLL_TOP:
        if (item->rot.x >= -45 * PHD_DEGREE) {
            item->rot.x -= UW_WALLDEFLECT;
        }
        break;

    case COLL_TOPFRONT:
        item->fall_speed = 0;
        break;

    case COLL_LEFT:
        item->rot.y += 5 * PHD_DEGREE;
        break;

    case COLL_RIGHT:
        item->rot.y -= 5 * PHD_DEGREE;
        break;

    case COLL_CLAMP:
        item->fall_speed = 0;
        return;
        break;
    }

    if (coll->mid_floor < 0) {
        item->pos.y += coll->mid_floor;
        item->rot.x += UW_WALLDEFLECT;
    }
}
