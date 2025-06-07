#include "game/lara/col.h"

#include "game/const.h"
#include "game/lara.h"
#include "game/sound.h"

static void M_Default(ITEM *item, COLL_INFO *coll);
static void M_Turn(ITEM *item, COLL_INFO *coll);
static void M_Death(ITEM *item, COLL_INFO *coll);
static void M_FastFall(ITEM *item, COLL_INFO *coll);
static void M_Reach(ITEM *item, COLL_INFO *coll);
static void M_Splat(ITEM *item, COLL_INFO *coll);
static void M_Compress(ITEM *item, COLL_INFO *coll);
static void M_Slide(ITEM *item, COLL_INFO *coll);
static void M_FallBack(ITEM *item, COLL_INFO *coll);

static void (*m_CollisionRoutines[])(ITEM *item, COLL_INFO *coll) = {
    // clang-format off
    [LS_WALK]         = Lara_Col_Walk,
    [LS_RUN]          = Lara_Col_Run,
    [LS_STOP]         = Lara_Col_Stop,
    [LS_JUMP_FORWARD] = Lara_Col_ForwardJump,
    [LS_POSE]         = Lara_Col_Stop,
    [LS_FAST_BACK]    = Lara_Col_FastBack,
    [LS_TURN_RIGHT]   = M_Turn,
    [LS_TURN_LEFT]    = M_Turn,
    [LS_DEATH]        = M_Death,
    [LS_FAST_FALL]    = M_FastFall,
    [LS_HANG]         = Lara_Col_Hang,
    [LS_REACH]        = M_Reach,
    [LS_SPLAT]        = M_Splat,
    [LS_TREAD]        = Lara_Col_Swim,
    [LS_LAND]         = Lara_Col_Stop,
    [LS_COMPRESS]     = M_Compress,
    [LS_BACK]         = Lara_Col_Back,
    [LS_SWIM]         = Lara_Col_Swim,
    [LS_GLIDE]        = Lara_Col_Swim,
    [LS_PULL_UP]      = M_Default,
    [LS_FAST_TURN]    = Lara_Col_Stop,
    [LS_STEP_RIGHT]   = Lara_Col_SideStep,
    [LS_STEP_LEFT]    = Lara_Col_SideStep,
    [LS_HIT]          = Lara_Col_Roll2,
    [LS_SLIDE]        = M_Slide,
    [LS_JUMP_BACK]    = Lara_Col_BackJump,
    [LS_JUMP_RIGHT]   = Lara_Col_RightJump,
    [LS_JUMP_LEFT]    = Lara_Col_LeftJump,
    [LS_JUMP_UP]      = Lara_Col_UpJump,
    [LS_FALL_BACK]    = M_FallBack,
    [LS_HANG_LEFT]    = Lara_Col_HangLeft,
    [LS_HANG_RIGHT]   = Lara_Col_HangRight,
    [LS_SLIDE_BACK]   = Lara_Col_SlideBack,
    [LS_SURF_TREAD]   = Lara_Col_SurfTread,
    [LS_SURF_SWIM]    = Lara_Col_SurfSwim,
    [LS_DIVE]         = Lara_Col_Swim,
    [LS_PUSH_BLOCK]   = M_Default,
    [LS_PULL_BLOCK]   = M_Default,
    [LS_PP_READY]     = M_Default,
    [LS_PICKUP]       = M_Default,
    [LS_SWITCH_ON]    = M_Default,
    [LS_SWITCH_OFF]   = M_Default,
    [LS_USE_KEY]      = M_Default,
    [LS_USE_PUZZLE]   = M_Default,
    [LS_UW_DEATH]     = Lara_Col_UWDeath,
    [LS_ROLL]         = Lara_Col_Roll,
    [LS_SPECIAL]      = nullptr,
    [LS_SURF_BACK]    = Lara_Col_SurfBack,
    [LS_SURF_LEFT]    = Lara_Col_SurfLeft,
    [LS_SURF_RIGHT]   = Lara_Col_SurfRight,
    [LS_USE_MIDAS]    = M_Default,
    [LS_DIE_MIDAS]    = M_Default,
    [LS_SWAN_DIVE]    = Lara_Col_SwanDive,
    [LS_FAST_DIVE]    = Lara_Col_FastDive,
    [LS_GYMNAST]      = M_Default,
    [LS_WATER_OUT]    = M_Default,
#if TR_VERSION == 1
    [LS_CONTROLLED]   = M_Default,
    [LS_TWIST]        = nullptr,
    [LS_WATER_ROLL]   = Lara_Col_Swim,
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
    [LS_WATER_ROLL]   = Lara_Col_Swim,
    [LS_FLARE_PICKUP] = M_Default,
    [LS_TWIST]        = nullptr,
    [LS_KICK]         = nullptr,
    [LS_ZIPLINE]      = nullptr,
#endif
    // clang-format on
};

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
    Lara_SlideSlope(item, coll);
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

void Lara_Col_Update(ITEM *const item, COLL_INFO *const coll)
{
    if (m_CollisionRoutines[item->current_anim_state] != nullptr) {
        m_CollisionRoutines[item->current_anim_state](item, coll);
    }
}
