#include "game/lara/misc.h"

#include "decomp/decomp.h"
#include "game/box.h"
#include "game/collide.h"
#include "game/effects.h"
#include "game/input.h"
#include "game/items.h"
#include "game/lara/control.h"
#include "game/math.h"
#include "game/matrix.h"
#include "game/random.h"
#include "game/room.h"
#include "game/sound.h"
#include "global/const.h"
#include "global/funcs.h"
#include "global/vars.h"

#include <libtrx/game/lara/misc.h>
#include <libtrx/game/math.h>
#include <libtrx/utils.h>

#define MAX_BADDIE_COLLISION 20
#define MOVE_SPEED 16
#define MOVE_ANGLE (2 * PHD_DEGREE) // = 364
#define CLIMB_HANG 900
#define CLIMB_SHIFT 70

static void __cdecl M_TakeHit(
    ITEM *const lara_item, const int32_t dx, const int32_t dz);

static void __cdecl M_TakeHit(
    ITEM *const lara_item, const int32_t dx, const int32_t dz)
{
    const PHD_ANGLE hit_angle = lara_item->rot.y + PHD_180 - Math_Atan(dz, dx);
    g_Lara.hit_direction = Math_GetDirection(hit_angle);
    if (g_Lara.hit_frame == 0) {
        Sound_Effect(SFX_LARA_INJURY, &lara_item->pos, SPM_NORMAL);
    }
    g_Lara.hit_frame++;
    CLAMPG(g_Lara.hit_frame, 34);
}

void __cdecl Lara_GetCollisionInfo(ITEM *item, COLL_INFO *coll)
{
    coll->facing = g_Lara.move_angle;
    Collide_GetCollisionInfo(
        coll, item->pos.x, item->pos.y, item->pos.z, item->room_num,
        LARA_HEIGHT);
}

void __cdecl Lara_SlideSlope(ITEM *item, COLL_INFO *coll)
{
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -STEP_L * 2;
    coll->bad_ceiling = 0;
    Lara_GetCollisionInfo(item, coll);

    if (Lara_HitCeiling(item, coll)) {
        return;
    }

    Lara_DeflectEdge(item, coll);

    if (coll->side_mid.floor > 200) {
        if (item->current_anim_state == LS_SLIDE) {
            item->goal_anim_state = LS_FORWARD_JUMP;
            item->current_anim_state = LS_FORWARD_JUMP;
            item->anim_num = LS_SURF_SWIM;
            item->frame_num = g_Anims[item->anim_num].frame_base;
        } else {
            item->goal_anim_state = LS_FALL_BACK;
            item->current_anim_state = LS_FALL_BACK;
            item->anim_num = LA_FALL_BACK;
            item->frame_num = g_Anims[item->anim_num].frame_base;
        }
        item->gravity = 1;
        item->fall_speed = 0;
        return;
    }

    Lara_TestSlide(item, coll);
    item->pos.y += coll->side_mid.floor;
    if (ABS(coll->x_tilt) <= 2 && ABS(coll->z_tilt) <= 2) {
        item->goal_anim_state = LS_STOP;
    }
}

int32_t __cdecl Lara_HitCeiling(ITEM *item, COLL_INFO *coll)
{
    if (coll->coll_type != COLL_TOP && coll->coll_type != COLL_CLAMP) {
        return 0;
    }

    item->pos.x = coll->old.x;
    item->pos.y = coll->old.y;
    item->pos.z = coll->old.z;
    item->goal_anim_state = LS_STOP;
    item->current_anim_state = LS_STOP;
    item->anim_num = LS_REACH;
    item->frame_num = g_Anims[item->anim_num].frame_base;
    item->speed = 0;
    item->gravity = 0;
    item->fall_speed = 0;
    return 1;
}

int32_t __cdecl Lara_DeflectEdge(ITEM *item, COLL_INFO *coll)
{
    switch (coll->coll_type) {
    case COLL_FRONT:
    case COLL_TOP_FRONT:
        Item_ShiftCol(item, coll);
        item->goal_anim_state = LS_STOP;
        item->current_anim_state = LS_STOP;
        item->gravity = 0;
        item->speed = 0;
        return 1;

    case COLL_LEFT:
        Item_ShiftCol(item, coll);
        item->rot.y += LARA_DEFLECT_ANGLE;
        return 0;

    case COLL_RIGHT:
        Item_ShiftCol(item, coll);
        item->rot.y -= LARA_DEFLECT_ANGLE;
        return 0;

    default:
        return 0;
    }
}

void __cdecl Lara_DeflectEdgeJump(ITEM *item, COLL_INFO *coll)
{
    Item_ShiftCol(item, coll);
    switch (coll->coll_type) {
    case COLL_FRONT:
    case COLL_TOP_FRONT:
        if (!g_Lara.climb_status || item->speed != 2) {
            if (coll->side_mid.floor > 512) {
                item->goal_anim_state = LS_FAST_FALL;
                item->current_anim_state = LS_FAST_FALL;
                item->anim_num = LA_SMASH_JUMP;
                item->frame_num = g_Anims[item->anim_num].frame_base + 1;
            } else if (coll->side_mid.floor <= 128) {
                item->goal_anim_state = LS_LAND;
                item->current_anim_state = LS_LAND;
                item->anim_num = LA_JUMP_UP_LAND;
                item->frame_num = g_Anims[item->anim_num].frame_base;
            }
            item->speed /= 4;
            g_Lara.move_angle += PHD_180;
            CLAMPL(item->fall_speed, 1);
        }
        break;

    case COLL_LEFT:
        item->rot.y += LARA_DEFLECT_ANGLE;
        break;

    case COLL_RIGHT:
        item->rot.y -= LARA_DEFLECT_ANGLE;
        break;

    case COLL_TOP:
        CLAMPL(item->fall_speed, 1);
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

    default:
        break;
    }
}

void __cdecl Lara_SlideEdgeJump(ITEM *item, COLL_INFO *coll)
{
    Item_ShiftCol(item, coll);

    switch (coll->coll_type) {
    case COLL_LEFT:
        item->rot.y += LARA_DEFLECT_ANGLE;
        break;

    case COLL_RIGHT:
        item->rot.y -= LARA_DEFLECT_ANGLE;
        break;

    case COLL_TOP:
    case COLL_TOP_FRONT:
        CLAMPL(item->fall_speed, 1);
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

    default:
        break;
    }
}

int32_t __cdecl Lara_TestWall(
    ITEM *item, int32_t front, int32_t right, int32_t down)
{
    int32_t x = item->pos.x;
    int32_t y = item->pos.y + down;
    int32_t z = item->pos.z;

    DIRECTION dir = Math_GetDirection(item->rot.y);
    switch (dir) {
    case DIR_NORTH:
        x -= right;
        break;
    case DIR_EAST:
        z -= right;
        break;
    case DIR_SOUTH:
        x += right;
        break;
    case DIR_WEST:
        z += right;
        break;
    default:
        break;
    }

    int16_t room_num = item->room_num;
    Room_GetSector(x, y, z, &room_num);

    switch (dir) {
    case DIR_NORTH:
        z += front;
        break;
    case DIR_EAST:
        x += front;
        break;
    case DIR_SOUTH:
        z -= front;
        break;
    case DIR_WEST:
        x -= front;
        break;
    default:
        break;
    }

    const SECTOR *const sector = Room_GetSector(x, y, z, &room_num);
    const int32_t height = Room_GetHeight(sector, x, y, z);
    const int32_t ceiling = Room_GetCeiling(sector, x, y, z);
    if (height == NO_HEIGHT) {
        return 1;
    }
    if (height - y > 0 && ceiling - y < 0) {
        return 0;
    }
    return 2;
}

int32_t __cdecl Lara_TestHangOnClimbWall(ITEM *item, COLL_INFO *coll)
{
    if (!g_Lara.climb_status || item->fall_speed < 0) {
        return 0;
    }

    DIRECTION dir = Math_GetDirection(item->rot.y);
    switch (dir) {
    case DIR_NORTH:
    case DIR_SOUTH:
        item->pos.z += coll->shift.z;
        break;

    case DIR_EAST:
    case DIR_WEST:
        item->pos.x += coll->shift.x;
        break;

    default:
        break;
    }

    const BOUNDS_16 *const bounds = Item_GetBoundsAccurate(item);
    int32_t y = bounds->min_y;
    int32_t h = bounds->max_y - y;

    int32_t shift;
    if (!Lara_TestClimbPos(item, coll->radius, coll->radius, y, h, &shift)) {
        return 0;
    }

    if (!Lara_TestClimbPos(item, coll->radius, -coll->radius, y, h, &shift)) {
        return 0;
    }

    int32_t result = Lara_TestClimbPos(item, coll->radius, 0, y, h, &shift);
    switch (result) {
    case 0:
    case 1:
        return result;

    default:
        item->pos.y += shift;
        return 1;
    }
}

int32_t __cdecl Lara_TestClimbStance(ITEM *item, COLL_INFO *coll)
{
    int32_t shift_r;
    int32_t result_r = Lara_TestClimbPos(
        item, coll->radius, coll->radius + LARA_CLIMB_WIDTH_RIGHT, -700,
        STEP_L * 2, &shift_r);
    if (result_r != 1) {
        return 0;
    }

    int32_t shift_l;
    int32_t result_l = Lara_TestClimbPos(
        item, coll->radius, -(coll->radius + LARA_CLIMB_WIDTH_LEFT), -700,
        STEP_L * 2, &shift_l);
    if (result_l != 1) {
        return 0;
    }

    int32_t shift = 0;
    if (shift_r) {
        if (shift_l) {
            if ((shift_r < 0) != (shift_l < 0)) {
                return 0;
            }
            if (shift_r < 0 && shift_l < shift_r) {
                shift = shift_l;
            } else if (shift_r > 0 && shift_l > shift_r) {
                shift = shift_l;
            } else {
                shift = shift_r;
            }
        } else {
            shift = shift_r;
        }
    } else if (shift_l) {
        shift = shift_l;
    }

    item->pos.y += shift;
    return 1;
}

void __cdecl Lara_HangTest(ITEM *item, COLL_INFO *coll)
{
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = NO_BAD_NEG;
    coll->bad_ceiling = 0;
    Lara_GetCollisionInfo(item, coll);
    bool flag = coll->side_front.floor < 200;

    item->gravity = 0;
    item->fall_speed = 0;
    g_Lara.move_angle = item->rot.y;

    DIRECTION dir = Math_GetDirection(item->rot.y);
    switch (dir) {
    case DIR_NORTH:
        item->pos.z += 2;
        break;
    case DIR_EAST:
        item->pos.x += 2;
        break;
    case DIR_SOUTH:
        item->pos.z -= 2;
        break;
    case DIR_WEST:
        item->pos.x -= 2;
        break;
    default:
        break;
    }

    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = 0;
    Lara_GetCollisionInfo(item, coll);

    if (g_Lara.climb_status) {
        if (!(g_Input & IN_ACTION) || item->hit_points <= 0) {
            XYZ_32 pos = {
                .x = 0,
                .y = 0,
                .z = 0,
            };
            Collide_GetJointAbsPosition(item, &pos, 0);
            if (dir == DIR_NORTH || dir == DIR_SOUTH) {
                item->pos.x = pos.x;
            } else {
                item->pos.z = pos.z;
            }

            item->goal_anim_state = LS_FORWARD_JUMP;
            item->current_anim_state = LS_FORWARD_JUMP;
            item->anim_num = LA_FALL_START;
            item->frame_num = g_Anims[item->anim_num].frame_base;
            item->pos.y += STEP_L;
            item->gravity = 1;
            item->speed = 2;
            item->fall_speed = 1;
            g_Lara.gun_status = LGS_ARMLESS;
            return;
        }

        if (!Lara_TestHangOnClimbWall(item, coll)) {
            item->pos.x = coll->old.x;
            item->pos.y = coll->old.y;
            item->pos.z = coll->old.z;
            item->goal_anim_state = LS_HANG;
            item->current_anim_state = LS_HANG;
            item->anim_num = LA_REACH_TO_HANG;
            item->frame_num = g_Anims[item->anim_num].frame_base + 21;
            return;
        }

        if (item->anim_num == LA_REACH_TO_HANG
            && item->frame_num == g_Anims[item->anim_num].frame_base + 21
            && Lara_TestClimbStance(item, coll)) {
            item->goal_anim_state = LS_CLIMB_STANCE;
        }
        return;
    }

    if (!(g_Input & IN_ACTION) || item->hit_points <= 0
        || coll->side_front.floor > 0) {
        item->goal_anim_state = LS_UP_JUMP;
        item->current_anim_state = LS_UP_JUMP;
        item->anim_num = LA_JUMP_UP;
        item->frame_num = g_Anims[item->anim_num].frame_base + 9;
        const BOUNDS_16 *const bounds = Item_GetBoundsAccurate(item);
        item->pos.y += bounds->max_y;
        item->pos.x += coll->shift.x;
        item->pos.z += coll->shift.z;
        item->gravity = 1;
        item->speed = 2;
        item->fall_speed = 1;
        g_Lara.gun_status = LGS_ARMLESS;
        return;
    }

    const BOUNDS_16 *const bounds = Item_GetBoundsAccurate(item);
    int32_t hdif = coll->side_front.floor - bounds->min_y;

    if (ABS(coll->side_left.floor - coll->side_right.floor) >= SLOPE_DIF
        || coll->side_mid.ceiling >= 0 || coll->coll_type != COLL_FRONT || flag
        || coll->hit_static || hdif < -SLOPE_DIF || hdif > SLOPE_DIF) {
        item->pos.x = coll->old.x;
        item->pos.y = coll->old.y;
        item->pos.z = coll->old.z;
        if (item->current_anim_state == LS_HANG_LEFT
            || item->current_anim_state == LS_HANG_RIGHT) {
            item->goal_anim_state = LS_HANG;
            item->current_anim_state = LS_HANG;
            item->anim_num = LA_REACH_TO_HANG;
            item->frame_num = g_Anims[item->anim_num].frame_base + 21;
        }
        return;
    }

    switch (dir) {
    case DIR_NORTH:
    case DIR_SOUTH:
        item->pos.z += coll->shift.z;
        break;

    case DIR_EAST:
    case DIR_WEST:
        item->pos.x += coll->shift.x;
        break;

    default:
        break;
    }

    item->pos.y += hdif;
}

int32_t __cdecl Lara_TestEdgeCatch(ITEM *item, COLL_INFO *coll, int32_t *edge)
{
    const BOUNDS_16 *const bounds = Item_GetBoundsAccurate(item);
    int32_t hdif1 = coll->side_front.floor - bounds->min_y;
    int32_t hdif2 = hdif1 + item->fall_speed;
    if ((hdif1 < 0 && hdif2 < 0) || (hdif1 > 0 && hdif2 > 0)) {
        hdif1 = item->pos.y + bounds->min_y;
        hdif2 = hdif1 + item->fall_speed;
        if ((hdif1 >> (WALL_SHIFT - 2)) == (hdif2 >> (WALL_SHIFT - 2))) {
            return 0;
        }
        if (item->fall_speed > 0) {
            *edge = hdif2 & ~(STEP_L - 1);
        } else {
            *edge = hdif1 & ~(STEP_L - 1);
        }
        return -1;
    }

    return ABS(coll->side_left.floor - coll->side_right.floor) < SLOPE_DIF;
}

int32_t __cdecl Lara_TestHangJumpUp(ITEM *item, COLL_INFO *coll)
{
    if (coll->coll_type != COLL_FRONT || !(g_Input & IN_ACTION)
        || g_Lara.gun_status != LGS_ARMLESS || coll->hit_static
        || coll->side_mid.ceiling > -STEPUP_HEIGHT) {
        return 0;
    }

    int32_t edge;
    int32_t edge_catch = Lara_TestEdgeCatch(item, coll, &edge);
    if (!edge_catch
        || (edge_catch < 0 && !Lara_TestHangOnClimbWall(item, coll))) {
        return 0;
    }

    DIRECTION dir = Math_GetDirectionCone(item->rot.y, LARA_HANG_ANGLE);
    if (dir == DIR_UNKNOWN) {
        return 0;
    }
    int16_t angle = Math_DirectionToAngle(dir);

    item->goal_anim_state = LS_HANG;
    item->current_anim_state = LS_HANG;
    item->anim_num = LA_REACH_TO_HANG;
    item->frame_num = g_Anims[item->anim_num].frame_base + 12;

    const BOUNDS_16 *const bounds = Item_GetBoundsAccurate(item);
    if (edge_catch > 0) {
        item->pos.y += coll->side_front.floor - bounds->min_y;
    } else {
        item->pos.y = edge - bounds->min_y;
    }
    item->pos.x += coll->shift.x;
    item->pos.z += coll->shift.z;
    item->rot.y = angle;
    item->speed = 0;
    item->gravity = 0;
    item->fall_speed = 0;
    g_Lara.gun_status = LGS_HANDS_BUSY;
    return 1;
}

int32_t __cdecl Lara_TestHangJump(ITEM *item, COLL_INFO *coll)
{
    if (coll->coll_type != COLL_FRONT || !(g_Input & IN_ACTION)
        || g_Lara.gun_status != LGS_ARMLESS || coll->hit_static
        || coll->side_mid.ceiling > -STEPUP_HEIGHT
        || coll->side_mid.floor < 200) {
        return 0;
    }

    int32_t edge;
    int32_t edge_catch = Lara_TestEdgeCatch(item, coll, &edge);
    if (!edge_catch
        || (edge_catch < 0 && !Lara_TestHangOnClimbWall(item, coll))) {
        return 0;
    }

    DIRECTION dir = Math_GetDirectionCone(item->rot.y, LARA_HANG_ANGLE);
    if (dir == DIR_UNKNOWN) {
        return 0;
    }
    int16_t angle = Math_DirectionToAngle(dir);

    if (Lara_TestHangSwingIn(item, angle)) {
        item->anim_num = LA_REACH_TO_THIN_LEDGE;
        item->frame_num = g_Anims[item->anim_num].frame_base;
    } else {
        item->anim_num = LA_REACH_TO_HANG;
        item->frame_num = g_Anims[item->anim_num].frame_base;
    }
    item->current_anim_state = LS_HANG;
    item->goal_anim_state = LS_HANG;

    const BOUNDS_16 *const bounds = Item_GetBoundsAccurate(item);
    if (edge_catch > 0) {
        item->pos.y += coll->side_front.floor - bounds->min_y;
        item->pos.x += coll->shift.x;
        item->pos.z += coll->shift.z;
    } else {
        item->pos.y = edge - bounds->min_y;
    }

    item->rot.y = angle;
    item->speed = 2;
    item->gravity = 1;
    item->fall_speed = 1;
    g_Lara.gun_status = LGS_HANDS_BUSY;
    return 1;
}

int32_t __cdecl Lara_TestHangSwingIn(ITEM *item, PHD_ANGLE angle)
{
    int32_t x = item->pos.x;
    int32_t y = item->pos.y;
    int32_t z = item->pos.z;
    int16_t room_num = item->room_num;
    switch (angle) {
    case 0:
        z += STEP_L;
        break;
    case PHD_90:
        x += STEP_L;
        break;
    case -PHD_180:
        z -= STEP_L;
        break;
    case -PHD_90:
        x -= STEP_L;
        break;
    }

    const SECTOR *const sector = Room_GetSector(x, y, z, &room_num);
    int32_t height = Room_GetHeight(sector, x, y, z);
    int32_t ceiling = Room_GetCeiling(sector, x, y, z);
    return height != NO_HEIGHT && height - y > 0 && ceiling - y < -400;
}

int32_t __cdecl Lara_TestVault(ITEM *item, COLL_INFO *coll)
{
    if (coll->coll_type != COLL_FRONT || !(g_Input & IN_ACTION)
        || g_Lara.gun_status != LGS_ARMLESS) {
        return 0;
    }

    DIRECTION dir = Math_GetDirectionCone(item->rot.y, LARA_VAULT_ANGLE);
    if (dir == DIR_UNKNOWN) {
        return 0;
    }
    int16_t angle = Math_DirectionToAngle(dir);

    int32_t left_floor = coll->side_left.floor;
    int32_t left_ceiling = coll->side_left.ceiling;
    int32_t right_floor = coll->side_right.floor;
    int32_t right_ceiling = coll->side_right.ceiling;
    int32_t front_floor = coll->side_front.floor;
    int32_t front_ceiling = coll->side_front.ceiling;
    bool slope = ABS(left_floor - right_floor) >= SLOPE_DIF;

    int32_t mid = STEP_L / 2;
    if (front_floor >= -STEP_L * 2 - mid && front_floor <= -STEP_L * 2 + mid) {
        if (slope || front_floor - front_ceiling < 0
            || left_floor - left_ceiling < 0
            || right_floor - right_ceiling < 0) {
            return 0;
        }
        item->goal_anim_state = LS_STOP;
        item->current_anim_state = LS_NULL;
        item->anim_num = LA_CLIMB_2CLICK;
        item->frame_num = g_Anims[item->anim_num].frame_base;
        item->pos.y += front_floor + STEP_L * 2;
        g_Lara.gun_status = LGS_HANDS_BUSY;
    } else if (
        front_floor >= -STEP_L * 3 - mid && front_floor <= -STEP_L * 3 + mid) {
        if (slope || front_floor - front_ceiling < 0
            || left_floor - left_ceiling < 0
            || right_floor - right_ceiling < 0) {
            return 0;
        }
        item->goal_anim_state = LS_STOP;
        item->current_anim_state = LS_NULL;
        item->anim_num = LA_CLIMB_3CLICK;
        item->frame_num = g_Anims[item->anim_num].frame_base;
        item->pos.y += front_floor + STEP_L * 3;
        g_Lara.gun_status = LGS_HANDS_BUSY;
    } else if (
        !slope && front_floor >= -STEP_L * 7 - mid
        && front_floor <= -STEP_L * 4 + mid) {
        item->goal_anim_state = LS_UP_JUMP;
        item->current_anim_state = LS_STOP;
        item->anim_num = LA_STAND_STILL;
        item->frame_num = g_Anims[item->anim_num].frame_base;
        g_Lara.calc_fallspeed =
            -(Math_Sqrt(-2 * GRAVITY * (front_floor + 800)) + 3);
        Lara_Animate(item);
    } else if (
        g_Lara.climb_status && front_floor <= -1920
        && g_Lara.water_status != LWS_WADE && left_floor <= -STEP_L * 8 + mid
        && right_floor <= -STEP_L * 8
        && coll->side_mid.ceiling <= -STEP_L * 8 + mid + LARA_HEIGHT) {
        item->goal_anim_state = LS_UP_JUMP;
        item->current_anim_state = LS_STOP;
        item->anim_num = LA_STAND_STILL;
        item->frame_num = g_Anims[item->anim_num].frame_base;
        g_Lara.calc_fallspeed = -116;
        Lara_Animate(item);
    } else if (
        g_Lara.climb_status
        && (front_floor < -STEP_L * 4 || front_ceiling >= LARA_HEIGHT - STEP_L)
        && coll->side_mid.ceiling <= -STEP_L * 5 + LARA_HEIGHT) {
        Item_ShiftCol(item, coll);
        if (Lara_TestClimbStance(item, coll)) {
            item->goal_anim_state = LS_CLIMB_STANCE;
            item->current_anim_state = LS_STOP;
            item->anim_num = LA_STAND_STILL;
            item->frame_num = g_Anims[item->anim_num].frame_base;
            Lara_Animate(item);
            item->rot.y = angle;
            g_Lara.gun_status = LGS_HANDS_BUSY;
            return 1;
        }
        return 0;
    } else {
        return 0;
    }

    item->rot.y = angle;
    Item_ShiftCol(item, coll);
    return 1;
}

int32_t __cdecl Lara_TestSlide(ITEM *item, COLL_INFO *coll)
{
    if (ABS(coll->x_tilt) <= 2 && ABS(coll->z_tilt) <= 2) {
        return 0;
    }

    int16_t angle = 0;
    if (coll->x_tilt > 2) {
        angle = -PHD_90;
    } else if (coll->x_tilt < -2) {
        angle = PHD_90;
    }

    if (coll->z_tilt > 2 && coll->z_tilt > ABS(coll->x_tilt)) {
        angle = PHD_180;
    } else if (coll->z_tilt < -2 && -coll->z_tilt > ABS(coll->x_tilt)) {
        angle = 0;
    }

    const int16_t angle_dif = angle - item->rot.y;
    Item_ShiftCol(item, coll);

    if (angle_dif >= -PHD_90 && angle_dif <= PHD_90) {
        if (item->current_anim_state == LS_SLIDE
            && g_LaraOldSlideAngle == angle) {
            return 1;
        }
        item->goal_anim_state = LS_SLIDE;
        item->current_anim_state = LS_SLIDE;
        item->anim_num = LA_SLIDE_FORWARD;
        item->frame_num = g_Anims[item->anim_num].frame_base;
        item->rot.y = angle;
    } else {
        if (item->current_anim_state == LS_SLIDE_BACK
            && g_LaraOldSlideAngle == angle) {
            return 1;
        }
        item->goal_anim_state = LS_SLIDE_BACK;
        item->current_anim_state = LS_SLIDE_BACK;
        item->anim_num = LA_SLIDE_BACKWARD_START;
        item->frame_num = g_Anims[item->anim_num].frame_base;
        item->rot.y = angle + PHD_180;
    }

    g_Lara.move_angle = angle;
    g_LaraOldSlideAngle = angle;
    return 1;
}

int16_t __cdecl Lara_FloorFront(ITEM *item, int16_t ang, int32_t dist)
{
    const int32_t x = item->pos.x + ((dist * Math_Sin(ang)) >> W2V_SHIFT);
    const int32_t y = item->pos.y - LARA_HEIGHT;
    const int32_t z = item->pos.z + ((dist * Math_Cos(ang)) >> W2V_SHIFT);
    int16_t room_num = item->room_num;
    const SECTOR *const sector = Room_GetSector(x, y, z, &room_num);
    int32_t height = Room_GetHeight(sector, x, y, z);
    if (height != NO_HEIGHT) {
        height -= item->pos.y;
    }
    return height;
}

int32_t __cdecl Lara_LandedBad(ITEM *item, COLL_INFO *coll)
{
    const int32_t x = item->pos.x;
    const int32_t y = item->pos.y;
    const int32_t z = item->pos.z;

    int16_t room_num = item->room_num;
    const SECTOR *const sector = Room_GetSector(x, y, z, &room_num);
    const int32_t height = Room_GetHeight(sector, x, y - LARA_HEIGHT, z);
    item->pos.y = height;
    item->floor = height;

    Room_TestTriggers(g_TriggerIndex, 0);
    int32_t land_speed = item->fall_speed - DAMAGE_START;
    item->pos.y = y;
    if (land_speed <= 0) {
        return 0;
    }
    if (land_speed <= DAMAGE_LENGTH) {
        item->hit_points += -LARA_MAX_HITPOINTS * land_speed * land_speed
            / (DAMAGE_LENGTH * DAMAGE_LENGTH);
    } else {
        item->hit_points = -1;
    }
    return item->hit_points <= 0;
}

int32_t __cdecl Lara_CheckForLetGo(ITEM *item, COLL_INFO *coll)
{
    item->gravity = 0;
    item->fall_speed = 0;

    int16_t room_num = item->room_num;
    int32_t x = item->pos.x;
    int32_t y = item->pos.y;
    int32_t z = item->pos.z;
    const SECTOR *const sector = Room_GetSector(x, y, z, &room_num);
    Room_GetHeight(sector, x, y, z);
    coll->trigger = g_TriggerIndex;
    if ((g_Input & IN_ACTION) && item->hit_points > 0) {
        return 0;
    }

    item->goal_anim_state = LS_FORWARD_JUMP;
    item->current_anim_state = LS_FORWARD_JUMP;
    item->anim_num = LA_FALL_START;
    item->frame_num = g_Anims[item->anim_num].frame_base;
    item->gravity = 1;
    item->speed = 2;
    item->fall_speed = 1;
    g_Lara.gun_status = LGS_ARMLESS;
    return 1;
}

void __cdecl Lara_GetJointAbsPosition(XYZ_32 *vec, int32_t joint)
{
    FRAME_INFO *frmptr[2] = { NULL, NULL };
    if (g_Lara.hit_direction < 0) {
        int32_t rate;
        int32_t frac = Item_GetFrames(g_LaraItem, frmptr, &rate);
        if (frac) {
            Lara_GetJointAbsPosition_I(
                g_LaraItem, vec, frmptr[0], frmptr[1], frac, rate);
            return;
        }
    }

    const FRAME_INFO *frame_ptr = NULL;
    const OBJECT *obj = &g_Objects[g_LaraItem->object_id];
    if (g_Lara.hit_direction >= 0) {
        LARA_ANIMATION anim_num;
        switch (g_Lara.hit_direction) {
        case DIR_EAST:
            anim_num = LA_HIT_RIGHT;
            break;
        case DIR_SOUTH:
            anim_num = LA_HIT_BACK;
            break;
        case DIR_WEST:
            anim_num = LA_HIT_LEFT;
            break;
        default:
            anim_num = LA_HIT_FRONT;
            break;
        }
        const ANIM *anim = &g_Anims[anim_num];
        int32_t interpolation = anim->interpolation;
        frame_ptr = (const FRAME_INFO *)(anim->frame_ptr
                                         + (int32_t)(g_Lara.hit_frame
                                                     * (interpolation >> 8)));
    } else {
        frame_ptr = frmptr[0];
    }

    Matrix_PushUnit();
    g_MatrixPtr->_03 = 0;
    g_MatrixPtr->_13 = 0;
    g_MatrixPtr->_23 = 0;
    Matrix_RotYXZ(g_LaraItem->rot.y, g_LaraItem->rot.x, g_LaraItem->rot.z);

    const int16_t *rot = frame_ptr->mesh_rots;
    const int32_t *bone = &g_AnimBones[obj->bone_idx];

    Matrix_TranslateRel(
        frame_ptr->offset.x, frame_ptr->offset.y, frame_ptr->offset.z);
    Matrix_RotYXZsuperpack(&rot, 0);

    Matrix_TranslateRel(bone[25], bone[26], bone[27]);
    Matrix_RotYXZsuperpack(&rot, 6);
    Matrix_RotYXZ(g_Lara.torso_y_rot, g_Lara.torso_x_rot, g_Lara.torso_z_rot);

    LARA_GUN_TYPE gun_type = LGT_UNARMED;
    if (g_Lara.gun_status == LGS_READY || g_Lara.gun_status == LGS_SPECIAL
        || g_Lara.gun_status == LGS_DRAW || g_Lara.gun_status == LGS_UNDRAW) {
        gun_type = g_Lara.gun_type;
    }

    if (g_Lara.gun_type == LGT_FLARE) {
        Matrix_TranslateRel(bone[41], bone[42], bone[43]);
        if (g_Lara.flare_control_left) {
            const LARA_ARM *arm = &g_Lara.left_arm;
            const ANIM *anim = &g_Anims[arm->anim_num];
            rot = &arm->frame_base
                       [(anim->interpolation >> 8)
                            * (arm->frame_num - anim->frame_base)
                        + FBBOX_ROT];
        } else {
            rot = frame_ptr->mesh_rots;
        }
        Matrix_RotYXZsuperpack(&rot, 11);

        Matrix_TranslateRel(bone[45], bone[46], bone[47]);
        Matrix_RotYXZsuperpack(&rot, 0);

        Matrix_TranslateRel(bone[49], bone[50], bone[51]);
        Matrix_RotYXZsuperpack(&rot, 0);
    } else if (gun_type != LGT_UNARMED) {
        Matrix_TranslateRel(bone[29], bone[30], bone[31]);

        const LARA_ARM *arm = &g_Lara.right_arm;
        const ANIM *anim = &g_Anims[arm->anim_num];
        rot = &arm->frame_base
                   [arm->frame_num * (anim->interpolation >> 8) + FBBOX_ROT];
        Matrix_RotYXZsuperpack(&rot, 8);

        Matrix_TranslateRel(bone[33], bone[34], bone[35]);
        Matrix_RotYXZsuperpack(&rot, 0);

        Matrix_TranslateRel(bone[37], bone[38], bone[39]);
        Matrix_RotYXZsuperpack(&rot, 0);
    }

    Matrix_TranslateRel(vec->x, vec->y, vec->z);
    vec->x = g_LaraItem->pos.x + (g_MatrixPtr->_03 >> W2V_SHIFT);
    vec->y = g_LaraItem->pos.y + (g_MatrixPtr->_13 >> W2V_SHIFT);
    vec->z = g_LaraItem->pos.z + (g_MatrixPtr->_23 >> W2V_SHIFT);
    Matrix_Pop();
}

void __cdecl Lara_GetJointAbsPosition_I(
    ITEM *item, XYZ_32 *vec, FRAME_INFO *frame1, FRAME_INFO *frame2,
    int32_t frac, int32_t rate)
{
    const OBJECT *obj = &g_Objects[item->object_id];

    Matrix_PushUnit();
    g_MatrixPtr->_03 = 0;
    g_MatrixPtr->_13 = 0;
    g_MatrixPtr->_23 = 0;
    Matrix_RotYXZ(item->rot.y, item->rot.x, item->rot.z);

    const int32_t *const bone = &g_AnimBones[obj->bone_idx];
    const int16_t *rot1 = frame1->mesh_rots;
    const int16_t *rot2 = frame2->mesh_rots;
    Matrix_InitInterpolate(frac, rate);

    Matrix_TranslateRel_ID(
        frame1->offset.x, frame1->offset.y, frame1->offset.z, frame2->offset.x,
        frame2->offset.y, frame2->offset.z);
    Matrix_RotYXZsuperpack_I(&rot1, &rot2, 0);

    Matrix_TranslateRel_I(bone[25], bone[26], bone[27]);
    Matrix_RotYXZsuperpack_I(&rot1, &rot2, 6);
    Matrix_RotYXZ_I(g_Lara.torso_y_rot, g_Lara.torso_x_rot, g_Lara.torso_z_rot);

    LARA_GUN_TYPE gun_type = LGT_UNARMED;
    if (g_Lara.gun_status == LGS_READY || g_Lara.gun_status == LGS_SPECIAL
        || g_Lara.gun_status == LGS_DRAW || g_Lara.gun_status == LGS_UNDRAW) {
        gun_type = g_Lara.gun_type;
    }

    if (g_Lara.gun_type == LGT_FLARE) {
        Matrix_Interpolate();
        Matrix_TranslateRel(bone[29], bone[30], bone[31]);
        if (g_Lara.flare_control_left) {
            const LARA_ARM *arm = &g_Lara.left_arm;
            const ANIM *anim = &g_Anims[arm->anim_num];
            rot1 = &arm->frame_base
                        [(anim->interpolation >> 8)
                             * (arm->frame_num - anim->frame_base)
                         + FBBOX_ROT];
        } else {
            rot1 = frame1->mesh_rots;
        }
        Matrix_RotYXZsuperpack(&rot1, 11);

        Matrix_TranslateRel(bone[45], bone[46], bone[47]);
        Matrix_RotYXZsuperpack(&rot1, 0);

        Matrix_TranslateRel(bone[49], bone[50], bone[51]);
        Matrix_RotYXZsuperpack(&rot1, 0);
    } else if (gun_type != LGT_UNARMED) {
        Matrix_Interpolate();
        Matrix_TranslateRel(bone[29], bone[30], bone[31]);

        const LARA_ARM *arm = &g_Lara.right_arm;
        const ANIM *anim = &g_Anims[arm->anim_num];
        rot1 = &arm->frame_base
                    [arm->frame_num * (anim->interpolation >> 8) + FBBOX_ROT];
        Matrix_RotYXZsuperpack(&rot1, 8);

        Matrix_TranslateRel(bone[33], bone[34], bone[35]);
        Matrix_RotYXZsuperpack(&rot1, 0);

        Matrix_TranslateRel(bone[37], bone[38], bone[39]);
        Matrix_RotYXZsuperpack(&rot1, 0);
    }

    Matrix_TranslateRel(vec->x, vec->y, vec->z);
    vec->x = item->pos.x + (g_MatrixPtr->_03 >> W2V_SHIFT);
    vec->y = item->pos.y + (g_MatrixPtr->_13 >> W2V_SHIFT);
    vec->z = item->pos.z + (g_MatrixPtr->_23 >> W2V_SHIFT);

    Matrix_Pop();
}

void __cdecl Lara_TakeHit(ITEM *const lara_item, const COLL_INFO *const coll)
{
    const int32_t dx = g_Lara.spaz_effect->pos.x - lara_item->pos.x;
    const int32_t dz = g_Lara.spaz_effect->pos.z - lara_item->pos.z;
    M_TakeHit(lara_item, dx, dz);
    g_Lara.spaz_effect_count--;
}

void __cdecl Lara_BaddieCollision(ITEM *lara_item, COLL_INFO *coll)
{
    lara_item->hit_status = 0;
    g_Lara.hit_direction = -1;
    if (lara_item->hit_points <= 0) {
        return;
    }

    int16_t roomies[MAX_BADDIE_COLLISION] = { 0 };
    int32_t roomies_count = 0;

    roomies[roomies_count++] = lara_item->room_num;

    PORTALS *portals = g_Rooms[roomies[0]].portals;
    if (portals != NULL) {
        for (int32_t i = 0; i < portals->count; i++) {
            if (roomies_count >= MAX_BADDIE_COLLISION) {
                break;
            }
            roomies[roomies_count++] = portals->portal[i].room_num;
        }
    }

    for (int32_t i = 0; i < roomies_count; i++) {
        int16_t item_num = g_Rooms[roomies[i]].item_num;
        while (item_num != NO_ITEM) {
            const ITEM *const item = &g_Items[item_num];

            // the collision routine can destroy the item - need to store the
            // next item beforehand
            const int16_t next_item_num = item->next_item;

            if (item->collidable && item->status != IS_INVISIBLE) {
                const OBJECT *const object = &g_Objects[item->object_id];
                if (object->collision) {
                    // clang-format off
                    const XYZ_32 d = {
                        .x = lara_item->pos.x - item->pos.x,
                        .y = lara_item->pos.y - item->pos.y,
                        .z = lara_item->pos.z - item->pos.z,
                    };
                    if (d.x > -TARGET_DIST && d.x < TARGET_DIST &&
                        d.y > -TARGET_DIST && d.y < TARGET_DIST &&
                        d.z > -TARGET_DIST && d.z < TARGET_DIST) {
                        object->collision(item_num, lara_item, coll);
                    }
                    // clang-format on
                }
            }

            item_num = next_item_num;
        }
    }

    if (g_Lara.spaz_effect_count) {
        Lara_TakeHit(lara_item, coll);
    }

    if (g_Lara.hit_direction == -1) {
        g_Lara.hit_frame = 0;
    }

    g_Inv_Chosen = -1;
}

void __cdecl Lara_Push(
    const ITEM *const item, ITEM *const lara_item, COLL_INFO *const coll,
    const bool spaz_on, const bool big_push)
{
    int32_t dx = lara_item->pos.x - item->pos.x;
    int32_t dz = lara_item->pos.z - item->pos.z;
    const int32_t c = Math_Cos(item->rot.y);
    const int32_t s = Math_Sin(item->rot.y);
    int32_t rx = (c * dx - s * dz) >> W2V_SHIFT;
    int32_t rz = (c * dz + s * dx) >> W2V_SHIFT;

    const BOUNDS_16 *const bounds = &Item_GetBestFrame(item)->bounds;
    int32_t min_x = bounds->min_x;
    int32_t max_x = bounds->max_x;
    int32_t min_z = bounds->min_z;
    int32_t max_z = bounds->max_z;

    if (big_push) {
        max_x += coll->radius;
        min_z -= coll->radius;
        max_z += coll->radius;
        min_x -= coll->radius;
    }

    if (rx < min_x || rx > max_x || rz < min_z || rz > max_z) {
        return;
    }

    int32_t l = rx - min_x;
    int32_t r = max_x - rx;
    int32_t t = max_z - rz;
    int32_t b = rz - min_z;

    if (l <= r && l <= t && l <= b) {
        rx -= l;
    } else if (r <= l && r <= t && r <= b) {
        rx += r;
    } else if (t <= l && t <= r && t <= b) {
        rz += t;
    } else {
        rz = min_z;
    }

    lara_item->pos.x = item->pos.x + ((rz * s + rx * c) >> W2V_SHIFT);
    lara_item->pos.z = item->pos.z + ((rz * c - rx * s) >> W2V_SHIFT);

    rz = (bounds->max_z + bounds->min_z) / 2;
    rx = (bounds->max_x + bounds->min_x) / 2;
    dx -= (c * rx + s * rz) >> W2V_SHIFT;
    dz -= (c * rz - s * rx) >> W2V_SHIFT;

    if (spaz_on && bounds->max_y - bounds->min_y > STEP_L) {
        M_TakeHit(lara_item, dx, dz);
    }

    int16_t old_facing = coll->facing;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = 0;
    coll->facing = Math_Atan(
        lara_item->pos.z - coll->old.z, lara_item->pos.x - coll->old.x);
    Collide_GetCollisionInfo(
        coll, lara_item->pos.x, lara_item->pos.y, lara_item->pos.z,
        lara_item->room_num, LARA_HEIGHT);
    coll->facing = old_facing;

    if (coll->coll_type != COLL_NONE) {
        lara_item->pos.x = coll->old.x;
        lara_item->pos.z = coll->old.z;
    } else {
        coll->old.x = lara_item->pos.x;
        coll->old.y = lara_item->pos.y;
        coll->old.z = lara_item->pos.z;
        Item_UpdateRoom(lara_item, -WALL_SHIFT);
    }
}

int32_t __cdecl Lara_MovePosition(XYZ_32 *vec, ITEM *item, ITEM *lara_item)
{
    const XYZ_16 rot = {
        .x = item->rot.x,
        .y = item->rot.y,
        .z = item->rot.z,
    };

    Matrix_PushUnit();
    Matrix_RotYXZ(rot.y, rot.x, rot.z);
    const MATRIX *const m = g_MatrixPtr;
    const XYZ_32 shift = {
        .x = (vec->y * m->_01 + vec->z * m->_02 + vec->x * m->_00) >> W2V_SHIFT,
        .y = (vec->x * m->_10 + vec->z * m->_12 + vec->y * m->_11) >> W2V_SHIFT,
        .z = (vec->y * m->_21 + vec->x * m->_20 + vec->z * m->_22) >> W2V_SHIFT,
    };
    Matrix_Pop();

    const XYZ_32 new_pos = {
        .x = item->pos.x + shift.x,
        .y = item->pos.y + shift.y,
        .z = item->pos.z + shift.z,
    };

    if (item->object_id == O_FLARE_ITEM) {
        int16_t room_num = lara_item->room_num;
        const SECTOR *const sector =
            Room_GetSector(new_pos.x, new_pos.y, new_pos.z, &room_num);
        const int32_t height =
            Room_GetHeight(sector, new_pos.x, new_pos.y, new_pos.z);
        if (ABS(height - lara_item->pos.y) > STEP_L * 2) {
            return false;
        }
        if (XYZ_32_GetDistance(&new_pos, &lara_item->pos) < STEP_L) {
            return true;
        }
    }

    // TODO: get rid of this conversion
    const PHD_3DPOS new_pos_full = {
        .pos = new_pos,
        .rot = rot,
    };
    PHD_3DPOS src_pos = {
        .pos = lara_item->pos,
        .rot = lara_item->rot,
    };
    const int32_t result =
        Misc_Move3DPosTo3DPos(&src_pos, &new_pos_full, MOVE_SPEED, MOVE_ANGLE);
    lara_item->pos = src_pos.pos;
    lara_item->rot = src_pos.rot;
    return result;
}

int32_t __cdecl Lara_IsNearItem(const XYZ_32 *const pos, const int32_t distance)
{
    return Item_IsNearItem(g_LaraItem, pos, distance);
}

int32_t __cdecl Lara_TestClimb(
    const int32_t x, const int32_t y, const int32_t z, const int32_t x_front,
    const int32_t z_front, const int32_t item_height, const int16_t item_room,
    int32_t *const shift)
{
    *shift = 0;
    bool hang = true;
    if (!g_Lara.climb_status) {
        return 0;
    }

    const SECTOR *sector;
    int32_t height;
    int32_t ceiling;

    int16_t room_num = item_room;
    sector = Room_GetSector(x, y - 128, z, &room_num);
    height = Room_GetHeight(sector, x, y, z);
    if (height == NO_HEIGHT) {
        return 0;
    }

    height -= y + item_height + STEP_L / 2;
    if (height < -CLIMB_SHIFT) {
        return 0;
    }
    if (height < 0) {
        *shift = height;
    }

    ceiling = Room_GetCeiling(sector, x, y, z) - y;
    if (ceiling > CLIMB_SHIFT) {
        return 0;
    }
    if (ceiling > 0) {
        if (*shift) {
            return 0;
        }
        *shift = ceiling;
    }

    if (item_height + height < CLIMB_HANG) {
        hang = false;
    }

    const int32_t x2 = x + x_front;
    const int32_t z2 = z + z_front;
    sector = Room_GetSector(x2, y, z2, &room_num);
    height = Room_GetHeight(sector, x2, y, z2);
    if (height != NO_HEIGHT) {
        height -= y;
    }

    if (height > CLIMB_SHIFT) {
        ceiling = Room_GetCeiling(sector, x2, y, z2) - y;
        if (ceiling >= LARA_CLIMB_HEIGHT) {
            return 1;
        }

        if (ceiling > LARA_CLIMB_HEIGHT - CLIMB_SHIFT) {
            if (*shift > 0) {
                return hang ? -1 : 0;
            }
            *shift = ceiling - LARA_CLIMB_HEIGHT;
            return 1;
        }

        if (ceiling > 0) {
            return hang ? -1 : 0;
        }

        if (ceiling > -CLIMB_SHIFT && hang && *shift <= 0) {
            if (*shift > ceiling) {
                *shift = ceiling;
            }

            return -1;
        }

        return 0;
    }

    if (height > 0) {
        if (*shift < 0) {
            return 0;
        }
        if (height > *shift) {
            *shift = height;
        }
    }

    room_num = item_room;
    sector = Room_GetSector(x, item_height + y, z, &room_num);
    sector = Room_GetSector(x2, item_height + y, z2, &room_num);
    ceiling = Room_GetCeiling(sector, x2, item_height + y, z2);
    if (ceiling == NO_HEIGHT) {
        return 1;
    }

    ceiling -= y;
    if (ceiling <= height) {
        return 1;
    }

    if (ceiling >= LARA_CLIMB_HEIGHT) {
        return 1;
    }

    if (ceiling > LARA_CLIMB_HEIGHT - CLIMB_SHIFT) {
        if (*shift > 0) {
            return hang ? -1 : 0;
        }
        *shift = ceiling - LARA_CLIMB_HEIGHT;
        return 1;
    }

    return hang ? -1 : 0;
}

int32_t __cdecl Lara_TestClimbPos(
    const ITEM *const item, const int32_t front, const int32_t right,
    const int32_t origin, const int32_t height, int32_t *const shift)
{
    const int32_t y = item->pos.y + origin;
    int32_t x;
    int32_t z;
    int32_t x_front = 0;
    int32_t z_front = 0;

    switch (Math_GetDirection(item->rot.y)) {
    case DIR_NORTH:
        x = item->pos.x + right;
        z = item->pos.z + front;
        z_front = 2;
        break;

    case DIR_EAST:
        x = item->pos.x + front;
        z = item->pos.z - right;
        x_front = 2;
        break;

    case DIR_SOUTH:
        x = item->pos.x - right;
        z = item->pos.z - front;
        z_front = -2;
        break;

    case DIR_WEST:
        x = item->pos.x - front;
        z = item->pos.z + right;
        x_front = -2;
        break;

    default:
        x = front;
        z = front;
        break;
    }

    return Lara_TestClimb(
        x, y, z, x_front, z_front, height, item->room_num, shift);
}

void __cdecl Lara_DoClimbLeftRight(
    ITEM *const item, const COLL_INFO *const coll, const int32_t result,
    const int32_t shift)
{
    if (result == 1) {
        if (g_Input & IN_LEFT) {
            item->goal_anim_state = LS_CLIMB_LEFT;
        } else if (g_Input & IN_RIGHT) {
            item->goal_anim_state = LS_CLIMB_RIGHT;
        } else {
            item->goal_anim_state = LS_CLIMB_STANCE;
        }
        item->pos.y += shift;
        return;
    }

    if (result) {
        item->goal_anim_state = LS_HANG;
        do {
            Item_Animate(item);
        } while (item->current_anim_state != LS_HANG);
        item->pos.x = coll->old.x;
        item->pos.z = coll->old.z;
        return;
    }

    item->pos.x = coll->old.x;
    item->pos.z = coll->old.z;
    item->goal_anim_state = LS_CLIMB_STANCE;
    item->current_anim_state = LS_CLIMB_STANCE;
    if (coll->old_anim_state == LS_CLIMB_STANCE) {
        item->frame_num = coll->old_frame_num;
        item->anim_num = coll->old_anim_num;
        Lara_Animate(item);
    } else {
        item->anim_num = LA_LADDER_IDLE;
        item->frame_num = g_Anims[item->anim_num].frame_base;
    }
}

int32_t __cdecl Lara_TestClimbUpPos(
    const ITEM *const item, const int32_t front, const int32_t right,
    int32_t *const shift, int32_t *const ledge)
{
    const int32_t y = item->pos.y - LARA_CLIMB_HEIGHT - STEP_L;
    int32_t x;
    int32_t z;
    int32_t x_front = 0;
    int32_t z_front = 0;

    switch (Math_GetDirection(item->rot.y)) {
    case DIR_NORTH:
        x = item->pos.x + right;
        z = item->pos.z + front;
        z_front = 2;
        break;

    case DIR_EAST:
        x = item->pos.x + front;
        z = item->pos.z - right;
        x_front = 2;
        break;

    case DIR_SOUTH:
        x = item->pos.x - right;
        z = item->pos.z - front;
        z_front = -2;
        break;

    case DIR_WEST:
        z = item->pos.z + right;
        x = item->pos.x - front;
        x_front = -2;
        break;

    default:
        x = front;
        z = front;
        break;
    }

    *shift = 0;

    const SECTOR *sector;
    int32_t height;
    int32_t ceiling;

    int16_t room_num = item->room_num;
    sector = Room_GetSector(x, y, z, &room_num);
    ceiling = Room_GetCeiling(sector, x, y, z) + STEP_L - y;
    if (ceiling > CLIMB_SHIFT) {
        return 0;
    }

    if (ceiling > 0) {
        *shift = ceiling;
    }

    const int32_t x2 = x + x_front;
    const int32_t z2 = z + z_front;
    sector = Room_GetSector(x2, y, z2, &room_num);
    height = Room_GetHeight(sector, x2, y, z2);
    if (height == NO_HEIGHT) {
        *ledge = NO_HEIGHT;
        return 1;
    }

    height -= y;
    *ledge = height;
    if (height > STEP_L / 2) {
        ceiling = Room_GetCeiling(sector, x2, y, z2) - y;
        if (ceiling >= LARA_CLIMB_HEIGHT) {
            return 1;
        }

        if (height - ceiling > LARA_HEIGHT) {
            *shift = height;
            return -1;
        }

        return 0;
    }

    if (height > 0 && height > *shift) {
        *shift = height;
    }

    room_num = item->room_num;
    sector = Room_GetSector(x, y + LARA_CLIMB_HEIGHT, z, &room_num);
    sector = Room_GetSector(x2, y + LARA_CLIMB_HEIGHT, z2, &room_num);
    ceiling = Room_GetCeiling(sector, x2, y + LARA_CLIMB_HEIGHT, z2) - y;
    if (ceiling <= height) {
        return 1;
    }

    if (ceiling >= LARA_CLIMB_HEIGHT) {
        return 1;
    }
    return 0;
}

int32_t __cdecl Lara_GetWaterDepth(
    const int32_t x, const int32_t y, const int32_t z, int16_t room_num)
{
    const ROOM *r = &g_Rooms[room_num];
    const SECTOR *sector;

    while (true) {
        int32_t z_sector = (z - r->pos.z) >> WALL_SHIFT;
        int32_t x_sector = (x - r->pos.x) >> WALL_SHIFT;

        if (z_sector <= 0) {
            z_sector = 0;
            if (x_sector < 1) {
                x_sector = 1;
            } else if (x_sector > r->size.x - 2) {
                x_sector = r->size.x - 2;
            }
        } else if (z_sector >= r->size.z - 1) {
            z_sector = r->size.z - 1;
            if (x_sector < 1) {
                x_sector = 1;
            } else if (x_sector > r->size.x - 2) {
                x_sector = r->size.x - 2;
            }
        } else if (x_sector < 0) {
            x_sector = 0;
        } else if (x_sector >= r->size.x) {
            x_sector = r->size.x - 1;
        }

        sector = &r->sectors[z_sector + x_sector * r->size.z];
        const int16_t data = Room_GetDoor(sector);
        if (data == NO_ROOM) {
            break;
        }
        room_num = data;
        r = &g_Rooms[room_num];
    }

    if (r->flags & RF_UNDERWATER) {
        while (sector->sky_room != NO_ROOM) {
            r = &g_Rooms[sector->sky_room];
            if (!(r->flags & RF_UNDERWATER)) {
                const int32_t water_height = sector->ceiling << 8;
                sector = Room_GetSector(x, y, z, &room_num);
                return Room_GetHeight(sector, x, y, z) - water_height;
            }
            const int32_t z_sector = (z - r->pos.z) >> WALL_SHIFT;
            const int32_t x_sector = (x - r->pos.x) >> WALL_SHIFT;
            sector = &r->sectors[z_sector + x_sector * r->size.z];
        }
        return 0x7FFF;
    }

    while (sector->pit_room != NO_ROOM) {
        r = &g_Rooms[sector->pit_room];
        if (r->flags & RF_UNDERWATER) {
            const int32_t water_height = sector->floor << 8;
            sector = Room_GetSector(x, y, z, &room_num);
            return Room_GetHeight(sector, x, y, z) - water_height;
        }
        const int32_t z_sector = (z - r->pos.z) >> WALL_SHIFT;
        const int32_t x_sector = (x - r->pos.x) >> WALL_SHIFT;
        sector = &r->sectors[z_sector + x_sector * r->size.z];
    }
    return NO_HEIGHT;
}

void __cdecl Lara_TestWaterDepth(ITEM *const item, const COLL_INFO *const coll)
{
    int16_t room_num = item->room_num;

    const SECTOR *const sector =
        Room_GetSector(item->pos.x, item->pos.y, item->pos.z, &room_num);
    const int32_t water_depth =
        Lara_GetWaterDepth(item->pos.x, item->pos.y, item->pos.z, room_num);

    if (water_depth == NO_HEIGHT) {
        item->pos = coll->old;
        item->fall_speed = 0;
    } else if (water_depth <= STEP_L * 2) {
        item->anim_num = LA_UNDERWATER_TO_STAND;
        item->frame_num = g_Anims[item->anim_num].frame_base;
        item->current_anim_state = LS_WATER_OUT;
        item->goal_anim_state = LS_STOP;
        item->rot.x = 0;
        item->rot.z = 0;
        item->gravity = 0;
        item->speed = 0;
        item->fall_speed = 0;
        g_Lara.water_status = LWS_WADE;
        item->pos.y =
            Room_GetHeight(sector, item->pos.x, item->pos.y, item->pos.z);
    }
}

void __cdecl Lara_SwimCollision(ITEM *const item, COLL_INFO *const coll)
{
    if (item->rot.x < -PHD_90 || item->rot.x > PHD_90) {
        g_Lara.move_angle = item->rot.y + PHD_180;
    } else {
        g_Lara.move_angle = item->rot.y;
    }

    coll->facing = g_Lara.move_angle;

    int32_t height = (LARA_HEIGHT * Math_Sin(item->rot.x)) >> W2V_SHIFT;
    if (height < 0) {
        height = -height;
    }
    CLAMPL(height, 200);

    coll->bad_neg = -height;
    Collide_GetCollisionInfo(
        coll, item->pos.x, item->pos.y + height / 2, item->pos.z,
        item->room_num, height);
    Item_ShiftCol(item, coll);

    switch (coll->coll_type) {
    case COLL_FRONT:
        if (item->rot.x > 35 * PHD_DEGREE) {
            item->rot.x += LARA_UW_WALL_DEFLECT;
        } else if (item->rot.x < -35 * PHD_DEGREE) {
            item->rot.x -= LARA_UW_WALL_DEFLECT;
        } else {
            item->fall_speed = 0;
        }
        break;

    case COLL_TOP:
        if (item->rot.x >= -45 * PHD_DEGREE) {
            item->rot.x -= LARA_UW_WALL_DEFLECT;
        }
        break;

    case COLL_TOP_FRONT:
        item->fall_speed = 0;
        break;

    case COLL_LEFT:
        item->rot.y += 5 * PHD_DEGREE;
        break;

    case COLL_RIGHT:
        item->rot.y -= 5 * PHD_DEGREE;
        break;

    case COLL_CLAMP:
        item->pos = coll->old;
        item->fall_speed = 0;
        return;
    }

    if (coll->side_mid.floor < 0) {
        item->rot.x += LARA_UW_WALL_DEFLECT;
        item->pos.y = coll->side_mid.floor + item->pos.y;
    }

    if (g_Lara.water_status != LWS_CHEAT && !g_Lara.extra_anim) {
        Lara_TestWaterDepth(item, coll);
    }
}

void __cdecl Lara_WaterCurrent(COLL_INFO *const coll)
{
    ITEM *const item = g_LaraItem;

    int16_t room_num = g_LaraItem->room_num;
    const ROOM *const r = &g_Rooms[g_LaraItem->room_num];
    const int32_t z_sector = (g_LaraItem->pos.z - r->pos.z) >> WALL_SHIFT;
    const int32_t x_sector = (g_LaraItem->pos.x - r->pos.x) >> WALL_SHIFT;
    g_LaraItem->box_num = r->sectors[z_sector + x_sector * r->size.z].box;

    if (g_Lara.creature == NULL) {
        g_Lara.current_active = 0;
        return;
    }

    XYZ_32 target;
    if (Box_CalculateTarget(&target, item, &g_Lara.creature->lot)
        == TARGET_NONE) {
        return;
    }

    target.x -= item->pos.x;
    if (target.x > g_Lara.current_active) {
        item->pos.x += g_Lara.current_active;
    } else if (target.x < -g_Lara.current_active) {
        item->pos.x -= g_Lara.current_active;
    } else {
        item->pos.x += target.x;
    }

    target.z -= item->pos.z;
    if (target.z > g_Lara.current_active) {
        item->pos.z += g_Lara.current_active;
    } else if (target.z < -g_Lara.current_active) {
        item->pos.z -= g_Lara.current_active;
    } else {
        item->pos.z += target.z;
    }

    target.y = target.y - item->pos.y;
    if (target.y > g_Lara.current_active) {
        item->pos.y += g_Lara.current_active;
    } else if (target.y < -g_Lara.current_active) {
        item->pos.y -= g_Lara.current_active;
    } else {
        item->pos.y += target.y;
    }

    g_Lara.current_active = 0;
    coll->facing =
        Math_Atan(item->pos.z - coll->old.z, item->pos.x - coll->old.x);
    Collide_GetCollisionInfo(
        coll, item->pos.x, item->pos.y + LARA_HEIGHT_UW / 2, item->pos.z,
        room_num, LARA_HEIGHT_UW);

    switch (coll->coll_type) {
    case COLL_FRONT:
        if (item->rot.x > 35 * PHD_DEGREE) {
            item->rot.x = item->rot.x + LARA_UW_WALL_DEFLECT;
        } else if (item->rot.x < -35 * PHD_DEGREE) {
            item->rot.x = item->rot.x - LARA_UW_WALL_DEFLECT;
        } else {
            item->fall_speed = 0;
        }
        break;

    case COLL_TOP:
        item->rot.x -= LARA_UW_WALL_DEFLECT;
        break;

    case COLL_TOP_FRONT:
        item->fall_speed = 0;
        break;

    case COLL_LEFT:
        item->rot.y += 910;
        break;

    case COLL_RIGHT:
        item->rot.y -= 910;
        break;

    default:
        break;
    }

    if (coll->side_mid.floor < 0) {
        item->pos.y += coll->side_mid.floor;
        item->rot.x += LARA_UW_WALL_DEFLECT;
    }
    Item_ShiftCol(item, coll);

    coll->old = item->pos;
}

void __cdecl Lara_CatchFire(void)
{
    if (g_Lara.burn || g_Lara.water_status == LWS_CHEAT) {
        return;
    }

    const int16_t fx_num = Effect_Create(g_LaraItem->room_num);
    if (fx_num == NO_ITEM) {
        return;
    }

    FX *const fx = &g_Effects[fx_num];
    fx->frame_num = 0;
    fx->object_id = O_FLAME;
    fx->counter = -1;
    g_Lara.burn = 1;
}

void __cdecl Lara_TouchLava(ITEM *const item)
{
    if (item->hit_points < 0 || g_Lara.water_status == LWS_CHEAT) {
        return;
    }

    int16_t room_num = item->room_num;
    const SECTOR *const sector =
        Room_GetSector(item->pos.x, MAX_HEIGHT, item->pos.z, &room_num);
    const int32_t height =
        Room_GetHeight(sector, item->pos.x, MAX_HEIGHT, item->pos.z);
    if (item->floor != height) {
        return;
    }

    item->hit_points = -1;
    item->hit_status = 1;

    for (int32_t i = 0; i < 10; i++) {
        const int16_t fx_num = Effect_Create(item->room_num);
        if (fx_num != NO_ITEM) {
            FX *const fx = &g_Effects[fx_num];
            fx->object_id = O_FLAME;
            fx->frame_num =
                g_Objects[O_FLAME].mesh_count * Random_GetControl() / 0x7FFF;
            fx->counter = -1 - 24 * Random_GetControl() / 0x7FFF;
        }
    }
}

void __cdecl Lara_Extinguish(void)
{
    if (!g_Lara.burn) {
        return;
    }

    g_Lara.burn = 0;

    // put out flame objects
    int16_t fx_num = g_NextEffectActive;
    while (fx_num != NO_ITEM) {
        FX *const fx = &g_Effects[fx_num];
        const int16_t next_fx_num = fx->next_active;
        if (fx->object_id == O_FLAME && fx->counter < 0) {
            fx->counter = 0;
            Effect_Kill(fx_num);
        }
        fx_num = next_fx_num;
    }
}
