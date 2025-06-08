#include "game/lara/col.h"

#include "config.h"
#include "debug.h"
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
#define M_LF_WALK_BACK_R_START 26
#define M_LF_WALK_BACK_R_END 55

#define M_LF_RUN_L_START 0
#define M_LF_RUN_L_HEEL_GROUND 3
#define M_LF_RUN_L_END 9
#define M_LF_RUN_R_START 10
#define M_LF_RUN_R_FOOT_GROUND 14
#define M_LF_RUN_R_END 21

static bool M_FixDescendingGlitch(void);
static bool M_FixStepGlitch(void);

static void M_Default(ITEM *item, COLL_INFO *coll);
static void M_Walk(ITEM *item, COLL_INFO *coll);
static void M_WalkBack(ITEM *item, COLL_INFO *coll);
static void M_SideStep(ITEM *item, COLL_INFO *coll);
static void M_Run(ITEM *item, COLL_INFO *coll);
static void M_Stop(ITEM *item, COLL_INFO *coll);
static void M_FastBack(ITEM *item, COLL_INFO *coll);
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
static void M_Roll(ITEM *item, COLL_INFO *coll);
static void M_RollContinue(ITEM *item, COLL_INFO *coll);
static void M_SwanDive(ITEM *item, COLL_INFO *coll);
static void M_FastDive(ITEM *item, COLL_INFO *coll);

static void (*m_CollisionRoutines[LS_NUMBER_OF])(
    ITEM *item, COLL_INFO *coll) = {
    // clang-format off
    [LS_WALK]         = M_Walk,
    [LS_RUN]          = M_Run,
    [LS_STOP]         = M_Stop,
    [LS_JUMP_FORWARD] = M_ForwardJump,
    [LS_POSE]         = M_Stop,
    [LS_FAST_BACK]    = M_FastBack,
    [LS_TURN_RIGHT]   = M_Turn,
    [LS_TURN_LEFT]    = M_Turn,
    [LS_DEATH]        = M_Death,
    [LS_FAST_FALL]    = M_FastFall,
    [LS_REACH]        = M_Reach,
    [LS_SPLAT]        = M_Splat,
    [LS_LAND]         = M_Stop,
    [LS_COMPRESS]     = M_Compress,
    [LS_WALK_BACK]    = M_WalkBack,
    [LS_PULL_UP]      = M_Default,
    [LS_FAST_TURN]    = M_Stop,
    [LS_STEP_RIGHT]   = M_SideStep,
    [LS_STEP_LEFT]    = M_SideStep,
    [LS_ROLL_CONT]    = M_RollContinue,
    [LS_SLIDE]        = M_Slide,
    [LS_JUMP_BACK]    = M_SideBackJump,
    [LS_JUMP_RIGHT]   = M_SideBackJump,
    [LS_JUMP_LEFT]    = M_SideBackJump,
    [LS_JUMP_UP]      = M_UpJump,
    [LS_FALL_BACK]    = M_FallBack,
    [LS_SLIDE_BACK]   = M_Slide,
    [LS_PUSH_BLOCK]   = M_Default,
    [LS_PULL_BLOCK]   = M_Default,
    [LS_PP_READY]     = M_Default,
    [LS_PICKUP]       = M_Default,
    [LS_SWITCH_ON]    = M_Default,
    [LS_SWITCH_OFF]   = M_Default,
    [LS_USE_KEY]      = M_Default,
    [LS_USE_PUZZLE]   = M_Default,
    [LS_ROLL]         = M_Roll,
    [LS_USE_MIDAS]    = M_Default,
    [LS_DIE_MIDAS]    = M_Default,
    [LS_SWAN_DIVE]    = M_SwanDive,
    [LS_FAST_DIVE]    = M_FastDive,
    [LS_GYMNAST]      = M_Default,
    [LS_WATER_OUT]    = M_Default,
#if TR_VERSION == 1
    [LS_CONTROLLED]   = M_Default,
#else
    [LS_FLARE_PICKUP] = M_Default,
#endif
    // clang-format on
};

static bool M_FixDescendingGlitch(void)
{
#if TR_VERSION == 1
    return g_Config.gameplay.fix_descending_glitch;
#else
    return true;
#endif
}

static bool M_FixStepGlitch(void)
{
#if TR_VERSION == 1
    return true;
#else
    return g_Config.gameplay.fix_step_glitch;
#endif
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
            Lara_Col_Stop(item, coll);
        }
    }

    if (Lara_Col_Fallen(item, coll)) {
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

static void M_WalkBack(ITEM *const item, COLL_INFO *const coll)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->move_angle = item->rot.y + DEG_180;
    item->gravity = false;
    item->fall_speed = 0;
    if (lara->water_status == LWS_WADE) {
        coll->bad_pos = NO_BAD_POS;
    } else {
        coll->bad_pos = STEPUP_HEIGHT;
    }
    coll->slopes_are_pits = 1;
    coll->slopes_are_walls = 1;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = 0;

    Lara_GetCollisionInfo(item, coll);
    if (Lara_HitCeiling(item, coll)) {
        return;
    }

    if (Lara_DeflectEdge(item, coll)) {
        Lara_Col_Stop(item, coll);
    }

    if (M_FixDescendingGlitch() && Lara_Col_Fallen(item, coll)) {
        return;
    }

    if (coll->side_mid.floor > STEP_L / 2
        && coll->side_mid.floor < STEPUP_HEIGHT) {
        if (Item_TestFrameRange(
                item, M_LF_WALK_BACK_R_START, M_LF_WALK_BACK_R_END)) {
            Item_SwitchToAnim(item, LA_WALK_DOWN_BACK_RIGHT, 0);
        } else {
            Item_SwitchToAnim(item, LA_WALK_DOWN_BACK_LEFT, 0);
        }
    }

    if (!Lara_TestSlide(item, coll)) {
        item->pos.y += coll->side_mid.floor;
    }
}

static void M_SideStep(ITEM *const item, COLL_INFO *const coll)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (item->current_anim_state == LS_STEP_RIGHT) {
        lara->move_angle = item->rot.y + DEG_90;
    } else {
        lara->move_angle = item->rot.y - DEG_90;
    }

    item->gravity = false;
    item->fall_speed = 0;
    if (lara->water_status == LWS_WADE) {
        coll->bad_pos = NO_BAD_POS;
    } else {
        coll->bad_pos = STEP_L / 2;
    }
    coll->slopes_are_pits = 1;
    coll->slopes_are_walls = 1;
    coll->bad_neg = -STEP_L / 2;
    coll->bad_ceiling = 0;

    Lara_GetCollisionInfo(item, coll);
    if (Lara_HitCeiling(item, coll)) {
        return;
    }

    if (Lara_DeflectEdge(item, coll)) {
        Lara_Col_Stop(item, coll);
    }

    if (M_FixDescendingGlitch() && Lara_Col_Fallen(item, coll)) {
        return;
    }

    if (!Lara_TestSlide(item, coll)) {
        item->pos.y += coll->side_mid.floor;
    }
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
        Lara_Col_Stop(item, coll);
    }

    if (Lara_Col_Fallen(item, coll)) {
        return;
    }

    if (coll->side_mid.floor >= -STEPUP_HEIGHT
        && coll->side_mid.floor < -STEP_L / 2) {
        if (M_FixStepGlitch()
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

static void M_Stop(ITEM *const item, COLL_INFO *const coll)
{
    item->gravity = false;
    item->fall_speed = 0;
    M_Default(item, coll);

    if (Lara_HitCeiling(item, coll)
        || (M_FixDescendingGlitch() && Lara_Col_Fallen(item, coll))
        || Lara_TestSlide(item, coll)) {
        return;
    }

    if (M_FixStepGlitch() && coll->side_mid.floor > 100) {
        item->current_anim_state = LS_JUMP_FORWARD;
        item->goal_anim_state = LS_JUMP_FORWARD;
        Item_SwitchToAnim(item, LA_FALL_START, 0);
        item->gravity = true;
        item->fall_speed = 0;
        return;
    }

    Lara_ShiftCol(coll);
    item->pos.y += coll->side_mid.floor;
}

static void M_FastBack(ITEM *const item, COLL_INFO *const coll)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->move_angle = item->rot.y + DEG_180;
    item->gravity = false;
    item->fall_speed = 0;
    coll->slopes_are_pits = 1;
    coll->slopes_are_walls = 1;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = 0;

    Lara_GetCollisionInfo(item, coll);
    if (Lara_HitCeiling(item, coll)) {
        return;
    }

    if (coll->side_mid.floor <= 200) {
        if (Lara_DeflectEdge(item, coll)) {
            Lara_Col_Stop(item, coll);
        }
        item->pos.y += coll->side_mid.floor;
    } else {
        Item_SwitchToAnim(item, LA_FALL_BACK, 0);
        item->current_anim_state = LS_FALL_BACK;
        item->goal_anim_state = LS_FALL_BACK;
        item->gravity = true;
        item->fall_speed = 0;
    }
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
    if (Lara_Col_Fallen(item, coll)) {
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

void Lara_Col_Register(
    const LARA_STATE state,
    void (*const handle_func)(ITEM *item, COLL_INFO *coll))
{
    ASSERT(state >= 0 && state < LS_NUMBER_OF);
    m_CollisionRoutines[state] = handle_func;
}

void Lara_Col_Update(ITEM *const item, COLL_INFO *const coll)
{
    if (m_CollisionRoutines[item->current_anim_state] != nullptr) {
        m_CollisionRoutines[item->current_anim_state](item, coll);
    }
}

bool Lara_Col_Fallen(ITEM *const item, const COLL_INFO *const coll)
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

void Lara_Col_Stop(ITEM *const item, const COLL_INFO *const coll)
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
