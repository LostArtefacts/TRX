#include "game/input.h"
#include "game/lara/common.h"
#include "game/lara/look.h"
#include "game/objects/common.h"
#include "game/objects/effects/twinkle.h"
#include "game/sound.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/game/camera.h>
#include <libtrx/game/lara.h>
#include <libtrx/game/lara/util.h>
#include <libtrx/game/math.h>

#include <stdint.h>

static void M_FastTurn(ITEM *item, COLL_INFO *coll);
static void M_StepRight(ITEM *item, COLL_INFO *coll);
static void M_StepLeft(ITEM *item, COLL_INFO *coll);
static void M_Slide(ITEM *item, COLL_INFO *coll);
static void M_SlideBack(ITEM *item, COLL_INFO *coll);
static void M_PushBlock(ITEM *item, COLL_INFO *coll);
static void M_PPReady(ITEM *item, COLL_INFO *coll);
static void M_Pickup(ITEM *item, COLL_INFO *coll);
static void M_SwitchOn(ITEM *item, COLL_INFO *coll);
static void M_UseKey(ITEM *item, COLL_INFO *coll);
static void M_Special(ITEM *item, COLL_INFO *coll);

static void M_FastTurn(ITEM *item, COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_STOP;
        return;
    }

    if (g_Config.gameplay.enable_enhanced_look && g_Input.look) {
        item->goal_anim_state = LS_STOP;
        return;
    }

    if (g_Lara.turn_rate >= 0) {
        g_Lara.turn_rate = LARA_FAST_TURN;
        if (!g_Input.right) {
            item->goal_anim_state = LS_STOP;
        }
    } else {
        g_Lara.turn_rate = -LARA_FAST_TURN;
        if (!g_Input.left) {
            item->goal_anim_state = LS_STOP;
        }
    }
}

static void M_StepRight(ITEM *item, COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_STOP;
        return;
    }

    if (!g_Input.step_right) {
        item->goal_anim_state = LS_STOP;
    }

    if (g_Input.left) {
        g_Lara.turn_rate -= LARA_TURN_RATE;
        if (g_Lara.turn_rate < -LARA_SLOW_TURN) {
            g_Lara.turn_rate = -LARA_SLOW_TURN;
        }
    } else if (g_Input.right) {
        g_Lara.turn_rate += LARA_TURN_RATE;
        if (g_Lara.turn_rate > LARA_SLOW_TURN) {
            g_Lara.turn_rate = LARA_SLOW_TURN;
        }
    }
}

static void M_StepLeft(ITEM *item, COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_STOP;
        return;
    }

    if (!g_Input.step_left) {
        item->goal_anim_state = LS_STOP;
    }

    if (g_Input.left) {
        g_Lara.turn_rate -= LARA_TURN_RATE;
        if (g_Lara.turn_rate < -LARA_SLOW_TURN) {
            g_Lara.turn_rate = -LARA_SLOW_TURN;
        }
    } else if (g_Input.right) {
        g_Lara.turn_rate += LARA_TURN_RATE;
        if (g_Lara.turn_rate > LARA_SLOW_TURN) {
            g_Lara.turn_rate = LARA_SLOW_TURN;
        }
    }
}

static void M_Slide(ITEM *item, COLL_INFO *coll)
{
    g_Camera.flags = CF_NO_CHUNKY;
    g_Camera.target_elevation = CAM_SLIDE_ELEVATION;
    if (g_Input.jump
        && (!g_Config.gameplay.enable_jump_twists || !g_Input.back)) {
        item->goal_anim_state = LS_JUMP_FORWARD;
    }
}

static void M_SlideBack(ITEM *item, COLL_INFO *coll)
{
    if (g_Input.jump
        && (!g_Config.gameplay.enable_jump_twists || !g_Input.forward)) {
        item->goal_anim_state = LS_JUMP_BACK;
    }
}

static void M_PushBlock(ITEM *item, COLL_INFO *coll)
{
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
    g_Camera.flags = CF_FOLLOW_CENTRE;
    g_Camera.target_angle = CAM_PUSH_BLOCK_ANGLE;
    g_Camera.target_elevation = CAM_PUSH_BLOCK_ELEVATION;
}

static void M_PPReady(ITEM *item, COLL_INFO *coll)
{
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_angle = CAM_PP_READY_ANGLE;
    if (!g_Input.action) {
        item->goal_anim_state = LS_STOP;
    }
}

static void M_Pickup(ITEM *item, COLL_INFO *coll)
{
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_angle = CAM_PICKUP_ANGLE;
    g_Camera.target_elevation = CAM_PICKUP_ELEVATION;
    g_Camera.target_distance = CAM_PICKUP_DISTANCE;
}

static void M_SwitchOn(ITEM *item, COLL_INFO *coll)
{
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_angle = CAM_SWITCH_ON_ANGLE;
    g_Camera.target_elevation = CAM_SWITCH_ON_ELEVATION;
    g_Camera.target_distance = CAM_SWITCH_ON_DISTANCE;
}

static void M_UseKey(ITEM *item, COLL_INFO *coll)
{
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_angle = CAM_USE_KEY_ANGLE;
    g_Camera.target_elevation = CAM_USE_KEY_ELEVATION;
    g_Camera.target_distance = CAM_USE_KEY_DISTANCE;
}

static void M_Special(ITEM *item, COLL_INFO *coll)
{
    ITEM *const target_item = Lara_GetDeathCameraTarget();
    if (target_item != nullptr) {
        g_Camera.item = target_item;
        g_Camera.flags = CF_CHASE_OBJECT;
        g_Camera.type = CAM_FIXED;
        g_Camera.target_angle = item->rot.y;
        g_Camera.target_distance = CAM_SPECIAL_DISTANCE;
    } else {
        g_Camera.flags = CF_FOLLOW_CENTRE;
        g_Camera.target_angle = CAM_SPECIAL_ANGLE;
    }
    g_Camera.target_elevation = CAM_SPECIAL_ELEVATION;
}

// clang-format off
REGISTER_LARA_STATE(LS_FAST_TURN,     M_FastTurn)
REGISTER_LARA_STATE(LS_STEP_RIGHT,    M_StepRight)
REGISTER_LARA_STATE(LS_STEP_LEFT,     M_StepLeft)
REGISTER_LARA_STATE(LS_SLIDE,         M_Slide)
REGISTER_LARA_STATE(LS_SLIDE_BACK,    M_SlideBack)
REGISTER_LARA_STATE(LS_PUSH_BLOCK,    M_PushBlock)
REGISTER_LARA_STATE(LS_PULL_BLOCK,    M_PushBlock)
REGISTER_LARA_STATE(LS_PP_READY,      M_PPReady)
REGISTER_LARA_STATE(LS_PICKUP,        M_Pickup)
REGISTER_LARA_STATE(LS_SWITCH_ON,     M_SwitchOn)
REGISTER_LARA_STATE(LS_SWITCH_OFF,    M_SwitchOn)
REGISTER_LARA_STATE(LS_USE_KEY,       M_UseKey)
REGISTER_LARA_STATE(LS_USE_PUZZLE,    M_UseKey)
REGISTER_LARA_STATE(LS_SPECIAL,       M_Special)
// clang-format on
