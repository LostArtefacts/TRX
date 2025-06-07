#include "game/lara/col.h"

#include "config.h"
#include "game/const.h"
#include "game/input.h"
#include "game/lara.h"
#include "game/rooms.h"
#include "game/sound.h"

static bool M_Fallen(ITEM *item, const COLL_INFO *coll);

static void M_Default(ITEM *item, COLL_INFO *coll);
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
static void M_Swim(ITEM *item, COLL_INFO *coll);
static void M_UWDeath(ITEM *item, COLL_INFO *coll);

static void (*m_CollisionRoutines[])(ITEM *item, COLL_INFO *coll) = {
    // clang-format off
    [LS_WALK]         = Lara_Col_Walk,
    [LS_RUN]          = Lara_Col_Run,
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
    [LS_SURF_TREAD]   = Lara_Col_SurfTread,
    [LS_SURF_SWIM]    = Lara_Col_SurfSwim,
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
    [LS_SURF_BACK]    = Lara_Col_SurfBack,
    [LS_SURF_LEFT]    = Lara_Col_SurfLeft,
    [LS_SURF_RIGHT]   = Lara_Col_SurfRight,
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
    [LS_WADE]         = Lara_Col_Wade,
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
    [LS_WADE]         = Lara_Col_Wade,
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

static void M_Swim(ITEM *const item, COLL_INFO *const coll)
{
    Lara_SwimCollision(item, coll);
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
    Lara_SwimCollision(item, coll);
}

void Lara_Col_Update(ITEM *const item, COLL_INFO *const coll)
{
    if (m_CollisionRoutines[item->current_anim_state] != nullptr) {
        m_CollisionRoutines[item->current_anim_state](item, coll);
    }
}
