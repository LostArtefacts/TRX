#include "game/lara/lara_misc.h"

#include "3dsystem/phd_math.h"
#include "game/collide.h"
#include "game/control.h"
#include "game/draw.h"
#include "game/input.h"
#include "game/lara/lara.h"
#include "global/vars.h"

void Lara_GetCollisionInfo(ITEM_INFO *item, COLL_INFO *coll)
{
    coll->facing = g_Lara.move_angle;
    GetCollisionInfo(
        coll, item->pos.x, item->pos.y, item->pos.z, item->room_number,
        LARA_HITE);
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

    g_Lara.move_angle = item->pos.y_rot;
    item->gravity_status = 0;
    item->fall_speed = 0;

    PHD_ANGLE angle = (uint16_t)(item->pos.y_rot + PHD_45) / PHD_90;
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
        item->anim_number = LA_STOP_HANG;
        item->frame_number = AF_STOPHANG;
        bounds = GetBoundsAccurate(item);
        item->pos.y += coll->front_floor - bounds[FRAME_BOUND_MIN_Y] + 2;
        item->pos.x += coll->shift.x;
        item->pos.z += coll->shift.z;
        item->gravity_status = 1;
        item->fall_speed = 1;
        item->speed = 2;
        g_Lara.gun_status = LGS_ARMLESS;
        return;
    }

    bounds = GetBoundsAccurate(item);
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
            item->anim_number = LA_HANG;
            item->frame_number = AF_HANG;
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
            item->anim_number = LA_FALL_DOWN;
            item->frame_number = AF_FALLDOWN;
        } else {
            item->current_anim_state = LS_FALL_BACK;
            item->goal_anim_state = LS_FALL_BACK;
            item->anim_number = LA_FALL_BACK;
            item->frame_number = AF_FALLBACK;
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
    item->anim_number = LA_FALL_DOWN;
    item->frame_number = g_Anims[item->anim_number].frame_base;
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
    item->anim_number = LA_STOP;
    item->frame_number = AF_STOP;
    item->gravity_status = 0;
    item->fall_speed = 0;
    item->speed = 0;
    return true;
}
bool Lara_DeflectEdge(ITEM_INFO *item, COLL_INFO *coll)
{
    if (coll->coll_type == COLL_FRONT || coll->coll_type == COLL_TOPFRONT) {
        ShiftItem(item, coll);
        item->goal_anim_state = LS_STOP;
        item->current_anim_state = LS_STOP;
        item->gravity_status = 0;
        item->speed = 0;
        return true;
    }

    if (coll->coll_type == COLL_LEFT) {
        ShiftItem(item, coll);
        item->pos.y_rot += LARA_DEF_ADD_EDGE;
    } else if (coll->coll_type == COLL_RIGHT) {
        ShiftItem(item, coll);
        item->pos.y_rot -= LARA_DEF_ADD_EDGE;
    }
    return false;
}

void Lara_DeflectEdgeJump(ITEM_INFO *item, COLL_INFO *coll)
{
    ShiftItem(item, coll);
    switch (coll->coll_type) {
    case COLL_LEFT:
        item->pos.y_rot += LARA_DEF_ADD_EDGE;
        break;

    case COLL_RIGHT:
        item->pos.y_rot -= LARA_DEF_ADD_EDGE;
        break;

    case COLL_FRONT:
    case COLL_TOPFRONT:
        item->goal_anim_state = LS_FAST_FALL;
        item->current_anim_state = LS_FAST_FALL;
        item->anim_number = LA_FAST_FALL;
        item->frame_number = AF_FASTFALL;
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
        item->pos.z -= (phd_cos(coll->facing) * 100) >> W2V_SHIFT;
        item->pos.x -= (phd_sin(coll->facing) * 100) >> W2V_SHIFT;
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
    ShiftItem(item, coll);
    switch (coll->coll_type) {
    case COLL_LEFT:
        item->pos.y_rot += LARA_DEF_ADD_EDGE;
        break;

    case COLL_RIGHT:
        item->pos.y_rot -= LARA_DEF_ADD_EDGE;
        break;

    case COLL_TOP:
    case COLL_TOPFRONT:
        if (item->fall_speed <= 0) {
            item->fall_speed = 1;
        }
        break;

    case COLL_CLAMP:
        item->pos.z -= (phd_cos(coll->facing) * 100) >> W2V_SHIFT;
        item->pos.x -= (phd_sin(coll->facing) * 100) >> W2V_SHIFT;
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

    PHD_ANGLE angle = item->pos.y_rot;
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
        item->anim_number = LA_VAULT_12;
        item->frame_number = AF_VAULT12;
        item->pos.y += STEP_L * 2 + hdif;
        g_Lara.gun_status = LGS_HANDS_BUSY;
        item->pos.y_rot = angle;
        ShiftItem(item, coll);
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
        item->anim_number = LA_VAULT_34;
        item->frame_number = AF_VAULT34;
        item->pos.y += STEP_L * 3 + hdif;
        g_Lara.gun_status = LGS_HANDS_BUSY;
        item->pos.y_rot = angle;
        ShiftItem(item, coll);
        return true;
    } else if (
        hdif >= -STEP_L * 7 - STEP_L / 2 && hdif <= -STEP_L * 4 + STEP_L / 2) {
        item->goal_anim_state = LS_JUMP_UP;
        item->current_anim_state = LS_STOP;
        item->anim_number = LA_STOP;
        item->frame_number = AF_STOP;
        g_Lara.calc_fall_speed =
            -(int16_t)(phd_sqrt((int)(-2 * GRAVITY * (hdif + 800))) + 3);
        Lara_Animate(item);
        item->pos.y_rot = angle;
        ShiftItem(item, coll);
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

    bounds = GetBoundsAccurate(item);
    hdif = coll->front_floor - bounds[FRAME_BOUND_MIN_Y];
    if (hdif < 0 && hdif + item->fall_speed < 0) {
        return false;
    }
    if (hdif > 0 && hdif + item->fall_speed > 0) {
        return false;
    }

    PHD_ANGLE angle = item->pos.y_rot;
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
        item->anim_number = LA_GRAB_LEDGE_IN;
        item->frame_number = AF_GRABLEDGEIN;
    } else {
        item->anim_number = LA_GRAB_LEDGE;
        item->frame_number = AF_GRABLEDGE;
    }
    item->current_anim_state = LS_HANG;
    item->goal_anim_state = LS_HANG;

    // bounds = GetBoundsAccurate(item);
    item->pos.y += hdif;
    item->pos.x += coll->shift.x;
    item->pos.z += coll->shift.z;
    item->pos.y_rot = angle;
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

    FLOOR_INFO *floor = GetFloor(x, y, z, &room_num);
    int h = GetHeight(floor, x, y, z);
    int c = GetCeiling(floor, x, y, z);

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

    bounds = GetBoundsAccurate(item);
    hdif = coll->front_floor - bounds[FRAME_BOUND_MIN_Y];
    if (hdif < 0 && hdif + item->fall_speed < 0) {
        return false;
    }
    if (hdif > 0 && hdif + item->fall_speed > 0) {
        return false;
    }

    PHD_ANGLE angle = item->pos.y_rot;
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
    item->anim_number = LA_HANG;
    item->frame_number = AF_STARTHANG;
    bounds = GetBoundsAccurate(item);
    item->pos.y += coll->front_floor - bounds[FRAME_BOUND_MIN_Y];
    item->pos.x += coll->shift.x;
    item->pos.z += coll->shift.z;
    item->pos.y_rot = angle;
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

    PHD_ANGLE adif = ang - item->pos.y_rot;
    ShiftItem(item, coll);
    if (adif >= -PHD_90 && adif <= PHD_90) {
        if (item->current_anim_state != LS_SLIDE || old_angle != ang) {
            item->goal_anim_state = LS_SLIDE;
            item->current_anim_state = LS_SLIDE;
            item->anim_number = LA_SLIDE;
            item->frame_number = AF_SLIDE;
            item->pos.y_rot = ang;
            g_Lara.move_angle = ang;
            old_angle = ang;
        }
    } else {
        if (item->current_anim_state != LS_SLIDE_BACK || old_angle != ang) {
            item->goal_anim_state = LS_SLIDE_BACK;
            item->current_anim_state = LS_SLIDE_BACK;
            item->anim_number = LA_SLIDE_BACK;
            item->frame_number = AF_SLIDEBACK;
            item->pos.y_rot = ang - PHD_180;
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
        GetFloor(item->pos.x, item->pos.y, item->pos.z, &room_num);

    int oy = item->pos.y;
    int height =
        GetHeight(floor, item->pos.x, item->pos.y - LARA_HITE, item->pos.z);

    item->floor = height;
    item->pos.y = height;
    TestTriggers(g_TriggerIndex, 0);
    item->pos.y = oy;

    int landspeed = item->fall_speed - DAMAGE_START;
    if (landspeed <= 0) {
        return false;
    } else if (landspeed > DAMAGE_LENGTH) {
        item->hit_points = -1;
    } else {
        item->hit_points -= (LARA_HITPOINTS * landspeed * landspeed)
            / (DAMAGE_LENGTH * DAMAGE_LENGTH);
    }

    if (item->hit_points < 0) {
        return true;
    }
    return false;
}

void Lara_SurfaceCollision(ITEM_INFO *item, COLL_INFO *coll)
{
    coll->facing = g_Lara.move_angle;

    GetCollisionInfo(
        coll, item->pos.x, item->pos.y + SURF_HITE, item->pos.z,
        item->room_number, SURF_HITE);

    ShiftItem(item, coll);

    if ((coll->coll_type
         & (COLL_FRONT | COLL_LEFT | COLL_RIGHT | COLL_TOP | COLL_TOPFRONT
            | COLL_CLAMP))
        || coll->mid_floor < 0) {
        item->fall_speed = 0;
        item->pos.x = coll->old.x;
        item->pos.y = coll->old.y;
        item->pos.z = coll->old.z;
    } else if (coll->coll_type == COLL_LEFT) {
        item->pos.y_rot += 5 * PHD_DEGREE;
    } else if (coll->coll_type == COLL_RIGHT) {
        item->pos.y_rot -= 5 * PHD_DEGREE;
    }

    int16_t wh = GetWaterHeight(
        item->pos.x, item->pos.y, item->pos.z, item->room_number);
    if (wh - item->pos.y <= -100) {
        item->goal_anim_state = LS_SWIM;
        item->current_anim_state = LS_DIVE;
        item->anim_number = LA_SURF_DIVE;
        item->frame_number = AF_SURFDIVE;
        item->pos.x_rot = -45 * PHD_DEGREE;
        item->fall_speed = 80;
        g_Lara.water_status = LWS_UNDERWATER;
        return;
    }

    Lara_TestWaterClimbOut(item, coll);
}

bool Lara_TestWaterClimbOut(ITEM_INFO *item, COLL_INFO *coll)
{
    if (item->pos.y_rot != g_Lara.move_angle) {
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

    PHD_ANGLE angle = item->pos.y_rot;
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
    UpdateLaraRoom(item, -LARA_HITE / 2);

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

    item->anim_number = LA_SURF_CLIMB;
    item->frame_number = AF_SURFCLIMB;
    item->current_anim_state = LS_WATER_OUT;
    item->goal_anim_state = LS_STOP;
    item->pos.x_rot = 0;
    item->pos.y_rot = angle;
    item->pos.z_rot = 0;
    item->gravity_status = 0;
    item->fall_speed = 0;
    item->speed = 0;
    g_Lara.gun_status = LGS_HANDS_BUSY;
    g_Lara.water_status = LWS_ABOVE_WATER;
    return true;
}

void Lara_SwimCollision(ITEM_INFO *item, COLL_INFO *coll)
{
    if (item->pos.x_rot >= -PHD_90 && item->pos.x_rot <= PHD_90) {
        g_Lara.move_angle = coll->facing = item->pos.y_rot;
    } else {
        g_Lara.move_angle = coll->facing = item->pos.y_rot - PHD_180;
    }
    GetCollisionInfo(
        coll, item->pos.x, item->pos.y + UW_HITE / 2, item->pos.z,
        item->room_number, UW_HITE);

    ShiftItem(item, coll);

    switch (coll->coll_type) {
    case COLL_FRONT:
        if (item->pos.x_rot > 35 * PHD_DEGREE) {
            item->pos.x_rot = item->pos.x_rot + UW_WALLDEFLECT;
            break;
        }
        if (item->pos.x_rot < -35 * PHD_DEGREE) {
            item->pos.x_rot = item->pos.x_rot - UW_WALLDEFLECT;
            break;
        }
        item->fall_speed = 0;
        break;

    case COLL_TOP:
        if (item->pos.x_rot >= -45 * PHD_DEGREE) {
            item->pos.x_rot -= UW_WALLDEFLECT;
        }
        break;

    case COLL_TOPFRONT:
        item->fall_speed = 0;
        break;

    case COLL_LEFT:
        item->pos.y_rot += 5 * PHD_DEGREE;
        break;

    case COLL_RIGHT:
        item->pos.y_rot -= 5 * PHD_DEGREE;
        break;

    case COLL_CLAMP:
        item->fall_speed = 0;
        return;
        break;
    }

    if (coll->mid_floor < 0) {
        item->pos.y += coll->mid_floor;
        item->pos.x_rot += UW_WALLDEFLECT;
    }
}
