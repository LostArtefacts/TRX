#include "game/lara/col.h"

#include "config.h"
#include "game/const.h"
#include "game/input.h"
#include "game/lara.h"
#include "game/rooms.h"
#include "game/sound.h"

#define M_LF_WALK_STEP_L_START 0
#define M_LF_WALK_STEP_L_NEAR_END 5
#define M_LF_WALK_STEP_L_END 6
#define M_LF_WALK_STEP_R_START 7
#define M_LF_WALK_STEP_R_MID 22
#define M_LF_WALK_STEP_R_NEAR_END 23
#define M_LF_WALK_STEP_R_END 25
#define M_LF_WALK_STEP_L_2_START 26
#define M_LF_WALK_STEP_L_2_END 35

#define M_LF_RUN_L_START 0
#define M_LF_RUN_L_HEEL_GROUND 3
#define M_LF_RUN_L_END 9
#define M_LF_RUN_R_START 10
#define M_LF_RUN_R_FOOT_GROUND 14
#define M_LF_RUN_R_END 21

#define M_LF_WADE_L_START 0
#define M_LF_WADE_L_END 9
#define M_LF_WADE_R_START 10
#define M_LF_WADE_R_END 21
#define M_LF_WADE_STEP_L_START 3
#define M_LF_WADE_STEP_L_END 14

static bool M_Fallen(ITEM *item, const COLL_INFO *coll);
static bool M_IsWadingEnabled(void);
static bool M_TestWaterStepOut(ITEM *item, const COLL_INFO *coll);
static bool M_TestWaterClimbOut(ITEM *item, const COLL_INFO *coll);
static void M_TestWaterDepth(ITEM *item, const COLL_INFO *coll);
static void M_CollideStop(ITEM *item, const COLL_INFO *coll);

static void M_Default(ITEM *item, COLL_INFO *coll);
static void M_Walk(ITEM *item, COLL_INFO *coll);
static void M_Run(ITEM *item, COLL_INFO *coll);
static void M_Turn(ITEM *item, COLL_INFO *coll);
static void M_Death(ITEM *item, COLL_INFO *coll);
static void M_FastFall(ITEM *item, COLL_INFO *coll);
static void M_Reach(ITEM *item, COLL_INFO *coll);
static void M_Splat(ITEM *item, COLL_INFO *coll);
static void M_Compress(ITEM *item, COLL_INFO *coll);
static void M_Slide(ITEM *item, COLL_INFO *coll);
static void M_ForwardJump(ITEM *item, COLL_INFO *coll);
static void M_UpJump(ITEM *item, COLL_INFO *coll);
static void M_SideBackJump(ITEM *item, COLL_INFO *coll);
static void M_FallBack(ITEM *item, COLL_INFO *coll);
static void M_Shimmy(ITEM *item, COLL_INFO *coll);
static void M_Roll(ITEM *item, COLL_INFO *coll);
static void M_RollContinue(ITEM *item, COLL_INFO *coll);
static void M_SwanDive(ITEM *item, COLL_INFO *coll);
static void M_FastDive(ITEM *item, COLL_INFO *coll);
static void M_CommonSurface(ITEM *item, COLL_INFO *coll);
static void M_ForwardSurface(ITEM *item, COLL_INFO *coll);
static void M_SideBackSurface(ITEM *item, COLL_INFO *coll);
static void M_Swim(ITEM *item, COLL_INFO *coll);
static void M_UWDeath(ITEM *item, COLL_INFO *coll);
static void M_Wade(ITEM *item, COLL_INFO *coll);

static void (*m_CollisionRoutines[])(ITEM *item, COLL_INFO *coll) = {
    // clang-format off
    [LS_WALK]         = M_Walk,
    [LS_RUN]          = M_Run,
    [LS_STOP]         = Lara_Col_Stop,
    [LS_JUMP_FORWARD] = M_ForwardJump,
    [LS_POSE]         = Lara_Col_Stop,
    [LS_FAST_BACK]    = Lara_Col_FastBack,
    [LS_TURN_RIGHT]   = M_Turn,
    [LS_TURN_LEFT]    = M_Turn,
    [LS_DEATH]        = M_Death,
    [LS_FAST_FALL]    = M_FastFall,
    [LS_HANG]         = Lara_Col_Hang,
    [LS_REACH]        = M_Reach,
    [LS_SPLAT]        = M_Splat,
    [LS_TREAD]        = M_Swim,
    [LS_LAND]         = Lara_Col_Stop,
    [LS_COMPRESS]     = M_Compress,
    [LS_BACK]         = Lara_Col_Back,
    [LS_SWIM]         = M_Swim,
    [LS_GLIDE]        = M_Swim,
    [LS_PULL_UP]      = M_Default,
    [LS_FAST_TURN]    = Lara_Col_Stop,
    [LS_STEP_RIGHT]   = Lara_Col_SideStep,
    [LS_STEP_LEFT]    = Lara_Col_SideStep,
    [LS_ROLL_CONT]    = M_RollContinue,
    [LS_SLIDE]        = M_Slide,
    [LS_JUMP_BACK]    = M_SideBackJump,
    [LS_JUMP_RIGHT]   = M_SideBackJump,
    [LS_JUMP_LEFT]    = M_SideBackJump,
    [LS_JUMP_UP]      = M_UpJump,
    [LS_FALL_BACK]    = M_FallBack,
    [LS_SHIMMY_LEFT]  = M_Shimmy,
    [LS_SHIMMY_RIGHT] = M_Shimmy,
    [LS_SLIDE_BACK]   = M_Slide,
    [LS_SURF_TREAD]   = M_SideBackSurface,
    [LS_SURF_SWIM]    = M_ForwardSurface,
    [LS_DIVE]         = M_Swim,
    [LS_PUSH_BLOCK]   = M_Default,
    [LS_PULL_BLOCK]   = M_Default,
    [LS_PP_READY]     = M_Default,
    [LS_PICKUP]       = M_Default,
    [LS_SWITCH_ON]    = M_Default,
    [LS_SWITCH_OFF]   = M_Default,
    [LS_USE_KEY]      = M_Default,
    [LS_USE_PUZZLE]   = M_Default,
    [LS_UW_DEATH]     = M_UWDeath,
    [LS_ROLL]         = M_Roll,
    [LS_SPECIAL]      = nullptr,
    [LS_SURF_BACK]    = M_SideBackSurface,
    [LS_SURF_LEFT]    = M_SideBackSurface,
    [LS_SURF_RIGHT]   = M_SideBackSurface,
    [LS_USE_MIDAS]    = M_Default,
    [LS_DIE_MIDAS]    = M_Default,
    [LS_SWAN_DIVE]    = M_SwanDive,
    [LS_FAST_DIVE]    = M_FastDive,
    [LS_GYMNAST]      = M_Default,
    [LS_WATER_OUT]    = M_Default,
#if TR_VERSION == 1
    [LS_CONTROLLED]   = M_Default,
    [LS_TWIST]        = nullptr,
    [LS_WATER_ROLL]   = M_Swim,
    [LS_WADE]         = M_Wade,
    [LS_RESPONSIVE]   = nullptr,
#else
    [LS_CLIMB_STANCE] = Lara_Col_ClimbStance,
    [LS_CLIMBING]     = Lara_Col_Climbing,
    [LS_CLIMB_LEFT]   = Lara_Col_ClimbLeft,
    [LS_CLIMB_END]    = nullptr,
    [LS_CLIMB_RIGHT]  = Lara_Col_ClimbRight,
    [LS_CLIMB_DOWN]   = Lara_Col_ClimbDown,
    [LS_LARA_TEST1]   = nullptr,
    [LS_LARA_TEST2]   = nullptr,
    [LS_LARA_TEST3]   = nullptr,
    [LS_WADE]         = M_Wade,
    [LS_WATER_ROLL]   = M_Swim,
    [LS_FLARE_PICKUP] = M_Default,
    [LS_TWIST]        = nullptr,
    [LS_KICK]         = nullptr,
    [LS_ZIPLINE]      = nullptr,
#endif
    // clang-format on
};

static bool M_Fallen(ITEM *const item, const COLL_INFO *const coll)
{
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    if (coll->side_mid.floor <= STEPUP_HEIGHT
        || lara->water_status == LWS_WADE) {
        return false;
    }
    item->current_anim_state = LS_JUMP_FORWARD;
    item->goal_anim_state = LS_JUMP_FORWARD;
    Item_SwitchToAnim(item, LA_FALL_START, 0);
    item->gravity = true;
    item->fall_speed = 0;
    return true;
}

static bool M_IsWadingEnabled(void)
{
#if TR_VERSION == 1
    return g_Config.gameplay.enable_wading;
#else
    return true;
#endif
}

static bool M_TestWaterStepOut(ITEM *const item, const COLL_INFO *const coll)
{
    if (coll->coll_type == COLL_FRONT || coll->side_mid.type == HT_BIG_SLOPE
        || coll->side_mid.floor >= 0) {
        return false;
    }

    if (coll->side_mid.floor < -STEP_L / 2) {
        item->current_anim_state = LS_WATER_OUT;
        item->goal_anim_state = LS_STOP;
        Item_SwitchToAnim(item, LA_ONWATER_TO_WADE, 0);
    } else if (item->goal_anim_state == LS_SURF_LEFT) {
        item->goal_anim_state = LS_STEP_LEFT;
    } else if (item->goal_anim_state == LS_SURF_RIGHT) {
        item->goal_anim_state = LS_STEP_RIGHT;
    } else {
        item->current_anim_state = LS_WADE;
        item->goal_anim_state = LS_WADE;
        Item_SwitchToAnim(item, LA_WADE, 0);
    }

    item->pos.y += coll->side_front.floor + LARA_HEIGHT_SURF - 5;
    Lara_UpdateRoomToHeight(-LARA_HEIGHT / 2);
    item->gravity = false;
    item->rot.x = 0;
    item->rot.z = 0;
    item->speed = 0;
    item->fall_speed = 0;
    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->water_status = LWS_WADE;
    return true;
}

static bool M_TestWaterClimbOut(ITEM *const item, const COLL_INFO *const coll)
{
    const int32_t coll_hdif =
        ABS(coll->side_left.floor - coll->side_right.floor);
    if (coll->coll_type != COLL_FRONT || !g_Input.action
        || coll_hdif >= SLOPE_DIF) {
        return false;
    }

    if (coll->side_front.ceiling > 0
        || coll->side_mid.ceiling > -STEPUP_HEIGHT) {
        return false;
    }

    LARA_INFO *const lara = Lara_GetLaraInfo();
#if TR_VERSION == 1
    if (item->rot.y != lara->move_angle) {
        return false;
    }
#else
    if (coll->side_front.type == HT_BIG_SLOPE) {
        return false;
    }
    if (lara->gun_status != LGS_ARMLESS
        && (lara->gun_status != LGS_READY || lara->gun_type != LGT_FLARE)) {
        return false;
    }
#endif

    const int32_t lara_hdif = coll->side_front.floor + LARA_HEIGHT_SURF;
    if (lara_hdif <= -STEP_L * 2
        || lara_hdif > LARA_HEIGHT_SURF - STEPUP_HEIGHT) {
        return false;
    }

    const DIRECTION dir = Math_GetDirectionCone(item->rot.y, LARA_HANG_ANGLE);
    if (dir == DIR_UNKNOWN) {
        return false;
    }

    item->pos.y += lara_hdif - 5;
    Lara_UpdateRoomToHeight(-LARA_HEIGHT / 2);

    switch (dir) {
    case DIR_NORTH:
        item->pos.z = (item->pos.z & -WALL_L) + WALL_L + LARA_RADIUS;
        break;
    case DIR_WEST:
        item->pos.x = (item->pos.x & -WALL_L) + WALL_L + LARA_RADIUS;
        break;
    case DIR_SOUTH:
        item->pos.z = (item->pos.z & -WALL_L) - LARA_RADIUS;
        break;
    case DIR_EAST:
        item->pos.x = (item->pos.x & -WALL_L) - LARA_RADIUS;
        break;
    case DIR_UNKNOWN:
        return false;
    }

    if (lara_hdif < -STEP_L / 2) {
        Item_SwitchToAnim(item, LA_ONWATER_TO_STAND_HIGH, 0);
    } else if (lara_hdif < STEP_L / 2) {
        Item_SwitchToAnim(item, LA_ONWATER_TO_STAND_MEDIUM, 0);
    } else {
        Item_SwitchToAnim(item, LA_ONWATER_TO_WADE_LOW, 0);
    }

    item->current_anim_state = LS_WATER_OUT;
    item->goal_anim_state = LS_STOP;
    item->rot.y = Math_DirectionToAngle(dir);
    item->rot.x = 0;
    item->rot.z = 0;
    item->gravity = false;
    item->speed = 0;
    item->fall_speed = 0;
    lara->gun_status = LGS_HANDS_BUSY;
    lara->water_status = LWS_ABOVE_WATER;
    return true;
}

static void M_TestWaterDepth(ITEM *const item, const COLL_INFO *const coll)
{
    int16_t room_num = item->room_num;
    const SECTOR *const sector =
        Room_GetSector(item->pos.x, item->pos.y, item->pos.z, &room_num);
    const int32_t water_depth =
        Lara_GetWaterDepth(item->pos.x, item->pos.y, item->pos.z, room_num);

    // TODO: offer ability for non-standard water exit in TR2. See #1782.
    if (TR_VERSION >= 2 && water_depth == NO_HEIGHT) {
        item->pos = coll->old;
        item->fall_speed = 0;
    } else if (water_depth != NO_HEIGHT && water_depth <= STEP_L * 2) {
        Item_SwitchToAnim(item, LA_UNDERWATER_TO_STAND, 0);
        item->current_anim_state = LS_WATER_OUT;
        item->goal_anim_state = LS_STOP;
        item->rot.x = 0;
        item->rot.z = 0;
        item->gravity = false;
        item->speed = 0;
        item->fall_speed = 0;
        LARA_INFO *const lara = Lara_GetLaraInfo();
        lara->water_status = LWS_WADE;
        item->pos.y =
            Room_GetHeight(sector, item->pos.x, item->pos.y, item->pos.z);
    }
}

static void M_CollideStop(ITEM *const item, const COLL_INFO *const coll)
{
#if TR_VERSION == 1
    // TODO: this routine gives smoother recovery after splatting against a wall
    // - offer it fully in TR1 as its only scope is currently for wading.
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    if (lara->water_status != LWS_WADE) {
        Item_SwitchToAnim(item, LA_STAND_STILL, 0);
        return;
    }
#endif

    switch (coll->old_anim_state) {
    case LS_STOP:
    case LS_TURN_RIGHT:
    case LS_TURN_LEFT:
    case LS_FAST_TURN:
        item->current_anim_state = coll->old_anim_state;
        item->anim_num = coll->old_anim_num;
        item->frame_num = coll->old_frame_num;
        if (g_Input.left) {
            item->goal_anim_state = LS_TURN_LEFT;
        } else if (g_Input.right) {
            item->goal_anim_state = LS_TURN_RIGHT;
        } else {
            item->goal_anim_state = LS_STOP;
        }
        Lara_Animate(item);
        break;

    default:
        Item_SwitchToAnim(item, LA_STAND_STILL, 0);
        break;
    }
}

static void M_Default(ITEM *const item, COLL_INFO *const coll)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->move_angle = item->rot.y;
    coll->bad_pos = STEPUP_HEIGHT;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = 0;
    coll->slopes_are_pits = 1;
    coll->slopes_are_walls = 1;
    Lara_GetCollisionInfo(item, coll);
}

static void M_Walk(ITEM *const item, COLL_INFO *const coll)
{
    item->gravity = false;
    item->fall_speed = 0;
    coll->lava_is_pit = 1;
    M_Default(item, coll);

    if (Lara_HitCeiling(item, coll) || Lara_TestVault(item, coll)) {
        return;
    }

    if (Lara_DeflectEdge(item, coll)) {
        if (Item_TestAnimEqual(item, LA_WALK_FORWARD)
            && Item_TestFrameRange(
                item, M_LF_WALK_STEP_R_START, M_LF_WALK_STEP_R_END)) {
            Item_SwitchToAnim(item, LA_WALK_STOP_RIGHT, 0);
        } else if (
            Item_TestAnimEqual(item, LA_WALK_FORWARD)
            && (Item_TestFrameRange(
                    item, M_LF_WALK_STEP_L_START, M_LF_WALK_STEP_L_END)
                || Item_TestFrameRange(
                    item, M_LF_WALK_STEP_L_2_START, M_LF_WALK_STEP_L_2_END))) {
            Item_SwitchToAnim(item, LA_WALK_STOP_LEFT, 0);
        } else {
            M_CollideStop(item, coll);
        }
    }

    if (M_Fallen(item, coll)) {
        return;
    }

    if (coll->side_mid.floor > STEP_L / 2) {
        if (Item_TestAnimEqual(item, LA_WALK_FORWARD)
            && Item_TestFrameRange(
                item, M_LF_WALK_STEP_L_END, M_LF_WALK_STEP_R_NEAR_END)) {
            Item_SwitchToAnim(item, LA_WALK_DOWN_LEFT, 0);
        } else {
            Item_SwitchToAnim(item, LA_WALK_DOWN_RIGHT, 0);
        }
    }

    if (coll->side_mid.floor >= -STEPUP_HEIGHT
        && coll->side_mid.floor < -STEP_L / 2) {
        if (Item_TestAnimEqual(item, LA_WALK_FORWARD)
            && Item_TestFrameRange(
                item, M_LF_WALK_STEP_L_NEAR_END, M_LF_WALK_STEP_R_MID)) {
            Item_SwitchToAnim(item, LA_WALK_UP_STEP_LEFT, 0);
        } else {
            Item_SwitchToAnim(item, LA_WALK_UP_STEP_RIGHT, 0);
        }
    }

    if (Lara_TestSlide(item, coll)) {
        return;
    }

    item->pos.y += coll->side_mid.floor;
}

static void M_Run(ITEM *const item, COLL_INFO *const coll)
{
    if (g_Config.gameplay.fix_qwop_glitch) {
        item->gravity = false;
        item->fall_speed = 0;
    }

    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->move_angle = item->rot.y;
    coll->slopes_are_walls = 1;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = 0;
    Lara_GetCollisionInfo(item, coll);

    if (Lara_HitCeiling(item, coll) || Lara_TestVault(item, coll)) {
        return;
    }

    if (Lara_DeflectEdge(item, coll)) {
        item->rot.z = 0;
        if (Lara_TestWall(item, STEP_L, 0, -STEP_L * 5 / 2)) {
            item->current_anim_state = LS_SPLAT;
            const bool is_run_anim = Item_TestAnimEqual(item, LA_RUN);
            if (is_run_anim
                && Item_TestFrameRange(
                    item, M_LF_RUN_L_START, M_LF_RUN_L_END)) {
                Item_SwitchToAnim(item, LA_WALL_SMASH_LEFT, 0);
                return;
            }
            if (is_run_anim
                && Item_TestFrameRange(
                    item, M_LF_RUN_R_START, M_LF_RUN_R_END)) {
                Item_SwitchToAnim(item, LA_WALL_SMASH_RIGHT, 0);
                return;
            }
        }
        M_CollideStop(item, coll);
    }

    if (M_Fallen(item, coll)) {
        return;
    }

#if TR_VERSION == 1
    const bool fix_step_glitch = true;
#else
    const bool fix_step_glitch = g_Config.gameplay.fix_step_glitch;
#endif
    if (coll->side_mid.floor >= -STEPUP_HEIGHT
        && coll->side_mid.floor < -STEP_L / 2) {
        if (fix_step_glitch
            && (coll->side_front.floor < -STEPUP_HEIGHT
                || coll->side_front.floor >= -STEP_L / 2)) {
            coll->side_mid.floor = 0;
        } else {
            if (Item_TestFrameRange(
                    item, M_LF_RUN_L_HEEL_GROUND, M_LF_RUN_R_FOOT_GROUND)) {
                Item_SwitchToAnim(item, LA_RUN_UP_STEP_LEFT, 0);
            } else {
                Item_SwitchToAnim(item, LA_RUN_UP_STEP_RIGHT, 0);
            }
        }
    }

    if (Lara_TestSlide(item, coll)) {
        return;
    }

    item->pos.y += MIN(coll->side_mid.floor, 50);
}

static void M_Turn(ITEM *const item, COLL_INFO *const coll)
{
    item->gravity = false;
    item->fall_speed = 0;
    M_Default(item, coll);

    if (coll->side_mid.floor <= 100) {
        if (!Lara_TestSlide(item, coll)) {
            item->pos.y += coll->side_mid.floor;
        }
    } else {
        Item_SwitchToAnim(item, LA_FALL_START, 0);
        item->current_anim_state = LS_JUMP_FORWARD;
        item->goal_anim_state = LS_JUMP_FORWARD;
        item->gravity = true;
        item->fall_speed = 0;
    }
}

static void M_Death(ITEM *const item, COLL_INFO *const coll)
{
#if TR_VERSION >= 2
    Sound_StopEffect(SFX_LARA_FALL);
#endif
    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->move_angle = item->rot.y;
    coll->bad_pos = STEPUP_HEIGHT;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = 0;
    coll->radius = LARA_RADIUS * 4;

    Lara_GetCollisionInfo(item, coll);
    Lara_ShiftCol(coll);

    item->pos.y += coll->side_mid.floor;
    item->hit_points = -1;
    lara->air = -1;
}

static void M_FastFall(ITEM *const item, COLL_INFO *const coll)
{
    item->gravity = true;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = BAD_JUMP_CEILING;

    Lara_GetCollisionInfo(item, coll);
    Lara_SlideEdgeJump(item, coll);
    if (coll->side_mid.floor > 0) {
        return;
    }

    if (Lara_LandedBad(item, coll)) {
        item->goal_anim_state = LS_DEATH;
    } else {
        item->goal_anim_state = LS_STOP;
        item->current_anim_state = LS_STOP;
        Item_SwitchToAnim(item, LA_FREEFALL_LAND, 0);
    }

    Sound_StopEffect(SFX_LARA_FALL);
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
    coll->bad_ceiling = BAD_JUMP_CEILING;

    Lara_GetCollisionInfo(item, coll);
    if (Lara_TestHangJump(item, coll)) {
        return;
    }

    Lara_SlideEdgeJump(item, coll);
    if (item->fall_speed <= 0 || coll->side_mid.floor > 0) {
        return;
    }

    if (Lara_LandedBad(item, coll)) {
        item->goal_anim_state = LS_DEATH;
    } else {
        item->goal_anim_state = LS_STOP;
    }
    item->gravity = false;
    item->fall_speed = 0;
    item->pos.y += coll->side_mid.floor;
}

static void M_Splat(ITEM *const item, COLL_INFO *const coll)
{
    M_Default(item, coll);
    Lara_ShiftCol(coll);
#if TR_VERSION >= 2
    if (coll->side_mid.floor > -STEP_L && coll->side_mid.floor < STEP_L) {
        item->pos.y += coll->side_mid.floor;
    }
#endif
}

static void M_Compress(ITEM *const item, COLL_INFO *const coll)
{
    item->gravity = false;
    item->fall_speed = 0;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = NO_BAD_NEG;
    coll->bad_ceiling = 0;

    Lara_GetCollisionInfo(item, coll);

    if (coll->side_mid.ceiling > -100) {
        Item_SwitchToAnim(item, LA_STAND_STILL, 0);
        item->goal_anim_state = LS_STOP;
        item->current_anim_state = LS_STOP;
        item->gravity = false;
        item->speed = 0;
        item->fall_speed = 0;
        item->pos = coll->old;
    }
#if TR_VERSION >= 2
    if (coll->side_mid.floor > -STEP_L && coll->side_mid.floor < STEP_L) {
        item->pos.y += coll->side_mid.floor;
    }
#endif
}

static void M_Slide(ITEM *const item, COLL_INFO *const coll)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->move_angle = item->rot.y;
    if (item->current_anim_state == LS_SLIDE_BACK) {
        lara->move_angle += DEG_180;
    }
    Lara_SlideSlope(item, coll);
}

static void M_ForwardJump(ITEM *const item, COLL_INFO *const coll)
{
#if TR_VERSION == 1
    // TODO: TR1's wall bug actually stems from Lara_DeflectEdgeJump, see about
    // fixing it there.
    const bool backward_momentum = false;
    const bool fix_wall_bug = g_Config.gameplay.fix_wall_jump_glitch;
#else
    const bool backward_momentum = item->speed < 0;
    const bool fix_wall_bug = false;
#endif

    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (backward_momentum) {
        lara->move_angle = item->rot.y + DEG_180;
    } else {
        lara->move_angle = item->rot.y;
    }
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = BAD_JUMP_CEILING;

    Lara_GetCollisionInfo(item, coll);
    Lara_DeflectEdgeJump(item, coll);
    if (backward_momentum) {
        lara->move_angle = item->rot.y;
    }

    if (coll->side_mid.floor > 0 || item->fall_speed <= 0) {
        return;
    }

    if (Lara_LandedBad(item, coll)) {
        item->goal_anim_state = LS_DEATH;
    } else if (
        lara->water_status != LWS_WADE && g_Input.forward && !g_Input.slow) {
        item->goal_anim_state = LS_RUN;
    } else {
        item->goal_anim_state = LS_STOP;
    }

    item->gravity = false;
    item->fall_speed = 0;
    item->pos.y += coll->side_mid.floor;
    item->speed = 0;
    if (!fix_wall_bug) {
        Lara_Animate(item);
    }
}

static void M_UpJump(ITEM *const item, COLL_INFO *const coll)
{
#if TR_VERSION == 1
    const bool enable_lean_jumping = g_Config.gameplay.enable_lean_jumping;
#else
    const bool enable_lean_jumping = true;
#endif
    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->move_angle = item->rot.y;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = BAD_JUMP_CEILING;
    coll->facing = lara->move_angle;
    if (enable_lean_jumping && item->speed < 0) {
        coll->facing += DEG_180;
    }

    Collide_GetCollisionInfo(
        coll, item->pos.x, item->pos.y, item->pos.z, item->room_num, 870);
    if (Lara_TestHangJumpUp(item, coll)) {
        return;
    }

    Lara_SlideEdgeJump(item, coll);
    if (enable_lean_jumping) {
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

    if (Lara_LandedBad(item, coll)) {
        item->goal_anim_state = LS_DEATH;
    } else {
        item->goal_anim_state = LS_STOP;
    }
    item->gravity = false;
    item->fall_speed = 0;
    item->pos.y += coll->side_mid.floor;
}

static void M_SideBackJump(ITEM *const item, COLL_INFO *const coll)
{
    int32_t angle = 0;
    switch (item->current_anim_state) {
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
    coll->bad_ceiling = BAD_JUMP_CEILING;

    Lara_GetCollisionInfo(item, coll);
    Lara_DeflectEdgeJump(item, coll);
    if (item->fall_speed <= 0 || coll->side_mid.floor > 0) {
        return;
    }

    if (Lara_LandedBad(item, coll)) {
        item->goal_anim_state = LS_DEATH;
    } else {
        item->goal_anim_state = LS_STOP;
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
    coll->bad_ceiling = BAD_JUMP_CEILING;

    Lara_GetCollisionInfo(item, coll);
    Lara_DeflectEdgeJump(item, coll);

    if (coll->side_mid.floor > 0 || item->fall_speed <= 0) {
        return;
    }

    if (Lara_LandedBad(item, coll)) {
        item->goal_anim_state = LS_DEATH;
    } else {
        item->goal_anim_state = LS_STOP;
    }

    item->gravity = false;
    item->fall_speed = 0;
    item->pos.y += coll->side_mid.floor;
}

static void M_Shimmy(ITEM *const item, COLL_INFO *const coll)
{
    const int32_t angle =
        item->current_anim_state == LS_SHIMMY_LEFT ? -DEG_90 : DEG_90;
    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->move_angle = item->rot.y + angle;
    Lara_HangTest(item, coll);
    lara->move_angle = item->rot.y + angle;
}

static void M_Roll(ITEM *const item, COLL_INFO *const coll)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->move_angle = item->rot.y;
    item->gravity = false;
    item->fall_speed = 0;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = 0;
    coll->slopes_are_walls = 1;

    Lara_GetCollisionInfo(item, coll);
    if (Lara_HitCeiling(item, coll) || Lara_TestSlide(item, coll)) {
        return;
    }

#if TR_VERSION == 1
    // TODO: offer as an option to allow roll-boosting off one-click steps.
    if (coll->side_mid.floor > 200) {
        item->current_anim_state = LS_JUMP_FORWARD;
        item->goal_anim_state = LS_JUMP_FORWARD;
        Item_SwitchToAnim(item, LA_FALL_START, 0);
        item->gravity = true;
        item->fall_speed = 0;
        return;
    }
#else
    if (M_Fallen(item, coll)) {
        return;
    }
#endif

    Lara_ShiftCol(coll);
    item->pos.y += coll->side_mid.floor;
}

static void M_RollContinue(ITEM *const item, COLL_INFO *const coll)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    item->gravity = false;
    item->fall_speed = 0;
    lara->move_angle = item->rot.y + DEG_180;
    coll->slopes_are_walls = 1;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = 0;

    Lara_GetCollisionInfo(item, coll);
    if (Lara_HitCeiling(item, coll) || Lara_TestSlide(item, coll)) {
        return;
    }

    if (coll->side_mid.floor > 200) {
        Item_SwitchToAnim(item, LA_FALL_BACK, 0);
        item->current_anim_state = LS_FALL_BACK;
        item->goal_anim_state = LS_FALL_BACK;
        item->gravity = true;
        item->fall_speed = 0;
    } else {
        Lara_ShiftCol(coll);
        item->pos.y += coll->side_mid.floor;
    }
}

static void M_SwanDive(ITEM *const item, COLL_INFO *const coll)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->move_angle = item->rot.y;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = BAD_JUMP_CEILING;

    Lara_GetCollisionInfo(item, coll);
    Lara_DeflectEdgeJump(item, coll);
    if (coll->side_mid.floor > 0 || item->fall_speed <= 0) {
        return;
    }

    item->goal_anim_state = LS_STOP;
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
    coll->bad_ceiling = BAD_JUMP_CEILING;

    Lara_GetCollisionInfo(item, coll);
    Lara_DeflectEdgeJump(item, coll);

    if (coll->side_mid.floor > 0 || item->fall_speed <= 0) {
        return;
    }

    if (item->fall_speed > 133) {
        item->goal_anim_state = LS_DEATH;
    } else {
        item->goal_anim_state = LS_STOP;
    }
    item->gravity = false;
    item->fall_speed = 0;
    item->pos.y += coll->side_mid.floor;
}

static void M_CommonSurface(ITEM *const item, COLL_INFO *const coll)
{
    const bool enable_wading = M_IsWadingEnabled();
    LARA_INFO *const lara = Lara_GetLaraInfo();
    coll->facing = lara->move_angle;

    int32_t obj_height = LARA_HEIGHT_SURF;
    if (enable_wading) {
        obj_height += 100;
    }
    Collide_GetCollisionInfo(
        coll, item->pos.x, item->pos.y + LARA_HEIGHT_SURF, item->pos.z,
        item->room_num, obj_height);

    Lara_ShiftCol(coll);

    if (coll->coll_type == COLL_LEFT) {
        item->rot.y += 5 * DEG_1;
    } else if (coll->coll_type == COLL_RIGHT) {
        item->rot.y -= 5 * DEG_1;
    } else if (
        coll->coll_type != COLL_NONE
        || (coll->side_mid.floor < 0 && coll->side_mid.type == HT_BIG_SLOPE)) {
        item->fall_speed = 0;
        item->pos = coll->old;
    }

    const int32_t water_height = Room_GetWaterHeight(
        item->pos.x, item->pos.y, item->pos.z, item->room_num);
    if (water_height - item->pos.y <= -100) {
        item->current_anim_state = LS_DIVE;
        item->goal_anim_state = LS_SWIM;
        Item_SwitchToAnim(item, LA_ONWATER_DIVE, 0);
        item->rot.x = -45 * DEG_1;
        item->fall_speed = 80;
        lara->water_status = LWS_UNDERWATER;
        return;
    }

    if (enable_wading) {
        M_TestWaterStepOut(item, coll);
    } else {
        M_TestWaterClimbOut(item, coll);
    }
}

static void M_ForwardSurface(ITEM *const item, COLL_INFO *const coll)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->move_angle = item->rot.y;
    coll->bad_neg = -STEPUP_HEIGHT;
    M_CommonSurface(item, coll);
    if (M_IsWadingEnabled()) {
        M_TestWaterClimbOut(item, coll);
    }
}

static void M_SideBackSurface(ITEM *const item, COLL_INFO *const coll)
{
    int32_t angle = 0;
    switch (item->current_anim_state) {
    case LS_SURF_BACK:
        angle = -DEG_180;
        break;
    case LS_SURF_LEFT:
        angle = -DEG_90;
        break;
    case LS_SURF_RIGHT:
        angle = DEG_90;
        break;
    }

    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->move_angle = item->rot.y + angle;
    M_CommonSurface(item, coll);
}

static void M_Swim(ITEM *const item, COLL_INFO *const coll)
{
    const bool enable_wading = M_IsWadingEnabled();
    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (item->rot.x < -DEG_90 || item->rot.x > DEG_90) {
        lara->move_angle = item->rot.y + DEG_180;
    } else {
        lara->move_angle = item->rot.y;
    }

    coll->facing = lara->move_angle;

    int32_t height;
    if (enable_wading) {
        height = (LARA_HEIGHT * Math_Sin(item->rot.x)) >> W2V_SHIFT;
        if (height < 0) {
            height = -height;
        }
        CLAMPL(height, 200);
        coll->bad_neg = -height;
    } else {
        height = LARA_HEIGHT_UW;
    }

    Collide_GetCollisionInfo(
        coll, item->pos.x, item->pos.y + height / 2, item->pos.z,
        item->room_num, height);
    Lara_ShiftCol(coll);

    switch (coll->coll_type) {
    case COLL_FRONT:
        if (item->rot.x > 35 * DEG_1) {
            item->rot.x += LARA_UW_WALL_DEFLECT;
        } else if (item->rot.x < -35 * DEG_1) {
            item->rot.x -= LARA_UW_WALL_DEFLECT;
        } else {
            item->fall_speed = 0;
        }
        break;

    case COLL_TOP:
        if (item->rot.x >= -45 * DEG_1) {
            item->rot.x -= LARA_UW_WALL_DEFLECT;
        }
        break;

    case COLL_TOP_FRONT:
        item->fall_speed = 0;
        break;

    case COLL_LEFT:
        item->rot.y += 5 * DEG_1;
        break;

    case COLL_RIGHT:
        item->rot.y -= 5 * DEG_1;
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

    if (enable_wading && lara->water_status != LWS_CHEAT && !lara->extra_anim) {
        M_TestWaterDepth(item, coll);
    }
}

static void M_UWDeath(ITEM *const item, COLL_INFO *const coll)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->air = -1;
    lara->gun_status = LGS_HANDS_BUSY;
    item->hit_points = -1;
    const int32_t water_height = Room_GetWaterHeight(
        item->pos.x, item->pos.y, item->pos.z, item->room_num);
    if (water_height != NO_HEIGHT && water_height < item->pos.y - 100) {
        item->pos.y -= 5;
    }
    M_Swim(item, coll);
}

static void M_Wade(ITEM *const item, COLL_INFO *const coll)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->move_angle = item->rot.y;
    coll->slopes_are_walls = 1;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = 0;

    Lara_GetCollisionInfo(item, coll);
    if (Lara_HitCeiling(item, coll) || Lara_TestVault(item, coll)) {
        return;
    }

    if (Lara_DeflectEdge(item, coll)) {
        item->rot.z = 0;
        if (coll->side_front.type == HT_WALL
            && coll->side_front.floor < -STEP_L * 5 / 2
            && coll->old_anim_state == LS_WADE
            && Item_TestAnimEqual(item, LA_WADE)) {
            item->current_anim_state = LS_SPLAT;
            if (Item_TestFrameRange(item, M_LF_WADE_L_START, M_LF_WADE_L_END)) {
                Item_SwitchToAnim(item, LA_WALL_SMASH_LEFT, 0);
                return;
            }
            if (Item_TestFrameRange(item, M_LF_WADE_R_START, M_LF_WADE_R_END)) {
                Item_SwitchToAnim(item, LA_WALL_SMASH_RIGHT, 0);
                return;
            }
        }
        M_CollideStop(item, coll);
    }

    if (M_Fallen(item, coll)) {
        return;
    }

    if (coll->side_mid.floor >= -STEPUP_HEIGHT
        && coll->side_mid.floor < -STEP_L / 2) {
        if (Item_TestFrameRange(
                item, M_LF_WADE_STEP_L_START, M_LF_WADE_STEP_L_END)) {
            Item_SwitchToAnim(item, LA_RUN_UP_STEP_LEFT, 0);
        } else {
            Item_SwitchToAnim(item, LA_RUN_UP_STEP_RIGHT, 0);
        }
    }

    if (Lara_TestSlide(item, coll)) {
        return;
    }

    item->pos.y += MIN(coll->side_mid.floor, 50);
}

void Lara_Col_Update(ITEM *const item, COLL_INFO *const coll)
{
    if (m_CollisionRoutines[item->current_anim_state] != nullptr) {
        m_CollisionRoutines[item->current_anim_state](item, coll);
    }
}
