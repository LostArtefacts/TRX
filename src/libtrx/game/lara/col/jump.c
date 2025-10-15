#include "config.h"
#include "game/input.h"
#include "game/lara.h"
#include "game/lara/util.h"
#include "game/rooms.h"
#include "game/sound.h"

// clang-format off
#define M_LF_START_HANG    12
#define M_LF_FAST_FALL     1
#define M_BAD_JUMP_CEILING ((STEP_L * 3) / 4) // = 192
// clang-format on

typedef enum {
    // clang-format off
    EDGE_CATCH_NEG  = -1,
    EDGE_CATCH_NONE = 0,
    EDGE_CATCH_POS  = 1,
    // clang-format on
} M_EDGE_CATCH;

static M_EDGE_CATCH M_TestEdgeCatch(
    const ITEM *const item, const COLL_INFO *const coll, int32_t *const edge)
{
    const BOUNDS_16 *const bounds = Item_GetBoundsAccurate(item);
    int32_t hdif1 = coll->side_front.floor - bounds->min.y;
    int32_t hdif2 = hdif1 + item->fall_speed;
    if ((hdif1 < 0 && hdif2 < 0) || (hdif1 > 0 && hdif2 > 0)) {
        hdif1 = item->pos.y + bounds->min.y;
        hdif2 = hdif1 + item->fall_speed;
        if ((hdif1 >> (WALL_SHIFT - 2)) == (hdif2 >> (WALL_SHIFT - 2))) {
            return EDGE_CATCH_NONE;
        }
        if (item->fall_speed > 0) {
            *edge = hdif2 & ~(STEP_L - 1);
        } else {
            *edge = hdif1 & ~(STEP_L - 1);
        }
        return EDGE_CATCH_NEG;
    }

    return ABS(coll->side_left.floor - coll->side_right.floor) < SLOPE_DIF
        ? EDGE_CATCH_POS
        : EDGE_CATCH_NONE;
}

static bool M_TestHangSwingIn(const ITEM *const item, const int16_t angle)
{
    int32_t x = item->pos.x;
    int32_t y = item->pos.y;
    int32_t z = item->pos.z;
    int16_t room_num = item->room_num;
    switch (angle) {
    case 0:
        z += STEP_L;
        break;
    case DEG_90:
        x += STEP_L;
        break;
    case -DEG_180:
        z -= STEP_L;
        break;
    case -DEG_90:
        x -= STEP_L;
        break;
    }

    const SECTOR *const sector = Room_GetSector(x, y, z, &room_num);
    int32_t height = Room_GetHeight(sector, x, y, z);
    int32_t ceiling = Room_GetCeiling(sector, x, y, z);
    return height != NO_HEIGHT && height - y > 0 && ceiling - y < -400;
}

static bool M_TestHangJump(ITEM *const item, COLL_INFO *const coll)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (coll->coll_type != COLL_FRONT || !g_Input.action
        || lara->gun_status != LGS_ARMLESS || coll->hit_static
        || coll->side_mid.ceiling > -STEPUP_HEIGHT
        || coll->side_mid.floor < 200) {
        return false;
    }

    int32_t edge;
    const M_EDGE_CATCH edge_catch = M_TestEdgeCatch(item, coll, &edge);
    if (edge_catch == EDGE_CATCH_NONE
        || (edge_catch == EDGE_CATCH_NEG
            && !Lara_Col_TestLadderHang(item, coll))) {
        return false;
    }

    const DIRECTION dir = Math_GetDirectionCone(item->rot.y, LARA_HANG_ANGLE);
    if (dir == DIR_UNKNOWN) {
        return false;
    }
    const int16_t angle = Math_DirectionToAngle(dir);

    if (M_TestHangSwingIn(item, angle)) {
        Item_SwitchToAnim(item, LA(LA_REACH_TO_THIN_LEDGE), 0);
    } else {
        Item_SwitchToAnim(item, LA(LA_REACH_TO_HANG), 0);
    }
    item->current_anim_state = LS(LS_HANG);
    item->goal_anim_state = LS(LS_HANG);

    const BOUNDS_16 *const bounds = Item_GetBoundsAccurate(item);
    if (edge_catch == EDGE_CATCH_POS) {
        item->pos.y += coll->side_front.floor - bounds->min.y;
        item->pos.x += coll->shift.x;
        item->pos.z += coll->shift.z;
    } else {
        item->pos.y = edge - bounds->min.y;
    }

    item->rot.y = angle;
    item->speed = 2;
    item->gravity = true;
    item->fall_speed = 1;
    lara->gun_status = LGS_HANDS_BUSY;
    return true;
}

static bool M_TestHangJumpUp(ITEM *const item, COLL_INFO *const coll)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (coll->coll_type != COLL_FRONT || !g_Input.action
        || lara->gun_status != LGS_ARMLESS || coll->hit_static
        || coll->side_mid.ceiling > -STEPUP_HEIGHT) {
        return false;
    }

    int32_t edge;
    const M_EDGE_CATCH edge_catch = M_TestEdgeCatch(item, coll, &edge);
    if (edge_catch == EDGE_CATCH_NONE
        || (edge_catch == EDGE_CATCH_NEG
            && !Lara_Col_TestLadderHang(item, coll))) {
        return false;
    }

    const DIRECTION dir = Math_GetDirectionCone(item->rot.y, LARA_HANG_ANGLE);
    if (dir == DIR_UNKNOWN) {
        return false;
    }
    const int16_t angle = Math_DirectionToAngle(dir);

    item->goal_anim_state = LS(LS_HANG);
    item->current_anim_state = LS(LS_HANG);
    Item_SwitchToAnim(item, LA(LA_REACH_TO_HANG), M_LF_START_HANG);

    const BOUNDS_16 *const bounds = Item_GetBoundsAccurate(item);
    if (edge_catch == EDGE_CATCH_POS) {
        item->pos.y += coll->side_front.floor - bounds->min.y;
    } else {
        item->pos.y = edge - bounds->min.y;
    }
    item->pos.x += coll->shift.x;
    item->pos.z += coll->shift.z;
    item->rot.y = angle;
    item->speed = 0;
    item->gravity = false;
    item->fall_speed = 0;
    lara->gun_status = LGS_HANDS_BUSY;
    return true;
}

static void M_SlideEdgeJump(ITEM *const item, COLL_INFO *const coll)
{
    Lara_Col_Shift(coll);

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
    }
}

static void M_Compress(ITEM *const item, COLL_INFO *const coll)
{
    item->gravity = false;
    item->fall_speed = 0;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = NO_BAD_NEG;
    coll->bad_ceiling = 0;

    Lara_Col_GetInfo(item, coll);

    if (coll->side_mid.ceiling > -100) {
        Item_SwitchToAnim(item, LA(LA_STAND_STILL), 0);
        item->goal_anim_state = LS(LS_STOP);
        item->current_anim_state = LS(LS_STOP);
        item->gravity = false;
        item->speed = 0;
        item->fall_speed = 0;
        item->pos = coll->old;
    }

    if (TR_VERSION >= 2 && coll->side_mid.floor > -STEP_L
        && coll->side_mid.floor < STEP_L) {
        item->pos.y += coll->side_mid.floor;
    }
}

static void M_NeutralJumpRoll(ITEM *const item, COLL_INFO *const coll)
{
    item->gravity = false;
    item->fall_speed = 0;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = NO_BAD_NEG;
    coll->bad_ceiling = 0;

    Lara_Col_GetInfo(item, coll);

    if (coll->side_mid.ceiling > -100) {
        Item_SwitchToAnim(item, LA(LA_STAND_STILL), 0);
        item->goal_anim_state = LS(LS_STOP);
        item->current_anim_state = LS(LS_STOP);
        item->speed = 0;
        item->pos = coll->old;
    }
}

static void M_UpJump(ITEM *const item, COLL_INFO *const coll)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->move_angle = item->rot.y;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = M_BAD_JUMP_CEILING;
    coll->facing = lara->move_angle;
    if (g_Config.gameplay.enable_lean_jumping && item->speed < 0) {
        coll->facing += DEG_180;
    }

    Collide_GetCollisionInfo(
        coll, item->pos.x, item->pos.y, item->pos.z, item->room_num, 870);
    if (M_TestHangJumpUp(item, coll)) {
        return;
    }

    M_SlideEdgeJump(item, coll);
    if (g_Config.gameplay.enable_lean_jumping) {
        if (coll->coll_type != COLL_NONE) {
            item->speed = item->speed > 0 ? 2 : -2;
        } else if (item->fall_speed < -70) {
            if (g_Input.forward && item->speed < 5) {
                item->speed++;
            } else if (g_Input.back && item->speed > -5) {
                item->speed -= 2;
            }
        }
    }

    if (item->fall_speed <= 0 || coll->side_mid.floor > 0) {
        return;
    }

    if (Lara_Col_LandedBad(item)) {
        item->goal_anim_state = LS(LS_DEATH);
    } else {
        item->goal_anim_state = LS(LS_STOP);
    }
    item->gravity = false;
    item->fall_speed = 0;
    item->pos.y += coll->side_mid.floor;
}

static void M_ForwardJump(ITEM *const item, COLL_INFO *const coll)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (item->speed < 0
        && g_Config.gameplay.wall_glitch_mode != WALL_GLITCH_TR1) {
        lara->move_angle = item->rot.y + DEG_180;
    } else {
        lara->move_angle = item->rot.y;
    }
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = M_BAD_JUMP_CEILING;

    Lara_Col_GetInfo(item, coll);
    Lara_Col_DeflectEdgeJump(item, coll);
    if (item->speed < 0
        && g_Config.gameplay.wall_glitch_mode != WALL_GLITCH_TR1) {
        lara->move_angle = item->rot.y;
    }

    if (coll->side_mid.floor > 0 || item->fall_speed <= 0) {
        return;
    }

    if (Lara_Col_LandedBad(item)) {
        item->goal_anim_state = LS(LS_DEATH);
    } else if (
        lara->water_status != LWS_WADE && g_Input.forward && !g_Input.slow) {
        item->goal_anim_state = LS(LS_RUN);
    } else {
        item->goal_anim_state = LS(LS_STOP);
    }

    item->gravity = false;
    item->fall_speed = 0;
    item->pos.y += coll->side_mid.floor;
    item->speed = 0;
    if (g_Config.gameplay.wall_glitch_mode != WALL_GLITCH_FIXED) {
        Lara_Animate(item);
    }
}

static void M_SideBackJump(ITEM *const item, COLL_INFO *const coll)
{
    int32_t angle = 0;
    switch (LS_U(item->current_anim_state)) {
    case LS_JUMP_BACK:
        angle = DEG_180;
        break;
    case LS_JUMP_RIGHT:
        angle = DEG_90;
        break;
    case LS_JUMP_LEFT:
        angle = -DEG_90;
        break;
    default:
        return;
    }

    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->move_angle = item->rot.y + angle;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = M_BAD_JUMP_CEILING;

    Lara_Col_GetInfo(item, coll);
    Lara_Col_DeflectEdgeJump(item, coll);
    if (item->fall_speed <= 0 || coll->side_mid.floor > 0) {
        return;
    }

    if (Lara_Col_LandedBad(item)) {
        item->goal_anim_state = LS(LS_DEATH);
    } else {
        item->goal_anim_state = LS(LS_STOP);
    }
    item->gravity = false;
    item->fall_speed = 0;
    item->pos.y += coll->side_mid.floor;
}

static void M_FallBack(ITEM *const item, COLL_INFO *const coll)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->move_angle = item->rot.y + DEG_180;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = M_BAD_JUMP_CEILING;

    Lara_Col_GetInfo(item, coll);
    Lara_Col_DeflectEdgeJump(item, coll);

    if (coll->side_mid.floor > 0 || item->fall_speed <= 0) {
        return;
    }

    if (Lara_Col_LandedBad(item)) {
        item->goal_anim_state = LS(LS_DEATH);
    } else {
        item->goal_anim_state = LS(LS_STOP);
    }

    item->gravity = false;
    item->fall_speed = 0;
    item->pos.y += coll->side_mid.floor;
}

static void M_Reach(ITEM *const item, COLL_INFO *const coll)
{
    item->gravity = true;
    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->move_angle = item->rot.y;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = 0;
    coll->bad_ceiling = M_BAD_JUMP_CEILING;

    Lara_Col_GetInfo(item, coll);
    if (M_TestHangJump(item, coll)) {
        return;
    }

    M_SlideEdgeJump(item, coll);
    if (item->fall_speed <= 0 || coll->side_mid.floor > 0) {
        return;
    }

    if (Lara_Col_LandedBad(item)) {
        item->goal_anim_state = LS(LS_DEATH);
    } else {
        item->goal_anim_state = LS(LS_STOP);
    }
    item->gravity = false;
    item->fall_speed = 0;
    item->pos.y += coll->side_mid.floor;
}

static void M_SwanDive(ITEM *const item, COLL_INFO *const coll)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->move_angle = item->rot.y;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = M_BAD_JUMP_CEILING;

    Lara_Col_GetInfo(item, coll);
    Lara_Col_DeflectEdgeJump(item, coll);
    if (coll->side_mid.floor > 0 || item->fall_speed <= 0) {
        return;
    }

    item->goal_anim_state = LS(LS_STOP);
    item->gravity = false;
    item->fall_speed = 0;
    item->pos.y += coll->side_mid.floor;
}

static void M_FastDive(ITEM *const item, COLL_INFO *const coll)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->move_angle = item->rot.y;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = M_BAD_JUMP_CEILING;

    Lara_Col_GetInfo(item, coll);
    Lara_Col_DeflectEdgeJump(item, coll);

    if (coll->side_mid.floor > 0 || item->fall_speed <= 0) {
        return;
    }

    if (item->fall_speed > 133 && !g_Config.debug.enable_invulnerability) {
        item->goal_anim_state = LS(LS_DEATH);
    } else {
        item->goal_anim_state = LS(LS_STOP);
    }
    item->gravity = false;
    item->fall_speed = 0;
    item->pos.y += coll->side_mid.floor;
}

static void M_FastFall(ITEM *const item, COLL_INFO *const coll)
{
    item->gravity = true;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = M_BAD_JUMP_CEILING;

    Lara_Col_GetInfo(item, coll);
    M_SlideEdgeJump(item, coll);
    if (coll->side_mid.floor > 0) {
        return;
    }

    if (Lara_Col_LandedBad(item)) {
        item->goal_anim_state = LS(LS_DEATH);
    } else {
        item->goal_anim_state = LS(LS_STOP);
        item->current_anim_state = LS(LS_STOP);
        Item_SwitchToAnim(item, LA(LA_FREEFALL_LAND), 0);
    }

    Sound_StopEffect(SFX_LARA_FALL);
    item->gravity = false;
    item->fall_speed = 0;
    item->pos.y += coll->side_mid.floor;
}

void Lara_Col_DeflectEdgeJump(ITEM *const item, COLL_INFO *const coll)
{
    Lara_Col_Shift(coll);
    switch (coll->coll_type) {
    case COLL_FRONT:
    case COLL_TOP_FRONT:
        LARA_INFO *const lara = Lara_GetLaraInfo();
        if (lara->climb_status && item->speed == 2) {
            break;
        }

        if (g_Config.gameplay.wall_glitch_mode == WALL_GLITCH_TR1
            || coll->side_mid.floor > (STEP_L * 2)) {
            item->goal_anim_state = LS(LS_FAST_FALL);
            item->current_anim_state = LS(LS_FAST_FALL);
            Item_SwitchToAnim(item, LA(LA_SMASH_JUMP), M_LF_FAST_FALL);
        } else if (coll->side_mid.floor <= (STEP_L / 2)) {
            item->goal_anim_state = LS(LS_LAND);
            item->current_anim_state = LS(LS_LAND);
            Item_SwitchToAnim(item, LA(LA_JUMP_UP_LAND), 0);
        }
        item->speed /= 4;
        lara->move_angle += DEG_180;
        CLAMPL(item->fall_speed, 1);
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
    }
}

bool Lara_Col_LandedBad(ITEM *const item)
{
    const XYZ_32 pos = item->pos;
    int16_t room_num = item->room_num;
    const SECTOR *const sector = Room_GetSector(pos.x, pos.y, pos.z, &room_num);
    const int32_t height =
        Room_GetHeight(sector, pos.x, pos.y - LARA_HEIGHT, pos.z);
    item->pos.y = height;
    item->floor = height;

    Room_TestTriggers(item);
    item->pos.y = pos.y;
    const int32_t land_speed = item->fall_speed - DAMAGE_START;
    if (land_speed <= 0) {
        return false;
    }

    if (g_Config.debug.enable_invulnerability) {
        return false;
    } else if (land_speed <= DAMAGE_LENGTH) {
        Lara_TakeDamage(
            LARA_MAX_HITPOINTS * SQUARE(land_speed) / SQUARE(DAMAGE_LENGTH),
            false);
    } else {
        item->hit_points = -1;
    }

    // #675: Original bug to keep. Correct operator would be <=
    return item->hit_points < 0;
}

// clang-format off
REGISTER_LARA_COL(LS_COMPRESS,     M_Compress)
REGISTER_LARA_COL(LS_NEUTRAL_ROLL, M_NeutralJumpRoll)
REGISTER_LARA_COL(LS_JUMP_UP,      M_UpJump)
REGISTER_LARA_COL(LS_JUMP_FORWARD, M_ForwardJump)
REGISTER_LARA_COL(LS_JUMP_BACK,    M_SideBackJump)
REGISTER_LARA_COL(LS_JUMP_RIGHT,   M_SideBackJump)
REGISTER_LARA_COL(LS_JUMP_LEFT,    M_SideBackJump)
REGISTER_LARA_COL(LS_FALL_BACK,    M_FallBack)
REGISTER_LARA_COL(LS_REACH,        M_Reach)
REGISTER_LARA_COL(LS_SWAN_DIVE,    M_SwanDive)
REGISTER_LARA_COL(LS_FAST_DIVE,    M_FastDive)
REGISTER_LARA_COL(LS_FAST_FALL,    M_FastFall)
// clang-format on
