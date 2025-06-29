#include "game/input.h"
#include "game/inventory.h"
#include "game/lara/control.h"
#include "game/lara/misc.h"
#include "game/overlay.h"
#include "game/sound.h"
#include "game/viewport.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/game/camera.h>
#include <libtrx/game/game.h>
#include <libtrx/game/lara.h>
#include <libtrx/game/lara/util.h>
#include <libtrx/game/music.h>
#include <libtrx/utils.h>

#define LF_FLARE_PICKUP_END 89
#define LF_UW_FLARE_PICKUP_END 35

static void M_Slide(ITEM *item, COLL_INFO *coll);
static void M_SlideBack(ITEM *item, COLL_INFO *coll);
static void M_PushBlock(ITEM *item, COLL_INFO *coll);
static void M_PPReady(ITEM *item, COLL_INFO *coll);
static void M_Pickup(ITEM *item, COLL_INFO *coll);
static void M_PickupFlare(ITEM *item, COLL_INFO *coll);
static void M_SwitchOn(ITEM *item, COLL_INFO *coll);
static void M_UseKey(ITEM *item, COLL_INFO *coll);
static void M_Special(ITEM *item, COLL_INFO *coll);

static void M_Slide(ITEM *item, COLL_INFO *coll)
{
    g_Camera.flags = CF_NO_CHUNKY;
    g_Camera.target_elevation = CAM_SLIDE_ELEVATION;
    if (g_Input.jump && !g_Input.back) {
        item->goal_anim_state = LS_JUMP_FORWARD;
    }
}

static void M_SlideBack(ITEM *item, COLL_INFO *coll)
{
    if (g_Input.jump && !g_Input.forward) {
        item->goal_anim_state = LS_JUMP_BACK;
    }
}

static void M_PushBlock(ITEM *item, COLL_INFO *coll)
{
    g_Lara.enable_look = false;
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
    g_Lara.enable_look = false;
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_angle = CAM_PICKUP_ANGLE;
    g_Camera.target_elevation = CAM_PICKUP_ELEVATION;
    g_Camera.target_distance = CAM_PICKUP_DISTANCE;
}

static void M_PickupFlare(ITEM *item, COLL_INFO *coll)
{
    M_Pickup(item, coll);
    const int16_t frame_num = Item_TestAnimEqual(item, LA_FLARE_PICKUP)
        ? LF_FLARE_PICKUP_END
        : LF_UW_FLARE_PICKUP_END;
    if (Item_TestFrameEqual(item, frame_num)) {
        g_Lara.gun_status = LGS_ARMLESS;
    }
}

static void M_SwitchOn(ITEM *item, COLL_INFO *coll)
{
    g_Lara.enable_look = false;
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_angle = CAM_SWITCH_ON_ANGLE;
    g_Camera.target_elevation = CAM_SWITCH_ON_ELEVATION;
    g_Camera.target_distance = CAM_SWITCH_ON_DISTANCE;
    g_Camera.speed = CAM_SWITCH_ON_SPEED;
}

static void M_UseKey(ITEM *item, COLL_INFO *coll)
{
    g_Lara.enable_look = false;
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_angle = CAM_USE_KEY_ANGLE;
    g_Camera.target_elevation = CAM_USE_KEY_ELEVATION;
    g_Camera.target_distance = CAM_USE_KEY_DISTANCE;
}

static void M_Special(ITEM *item, COLL_INFO *coll)
{
    g_Camera.flags = CF_FOLLOW_CENTRE;
    g_Camera.target_angle = CAM_SPECIAL_ANGLE;
    g_Camera.target_elevation = CAM_SPECIAL_ELEVATION;
}

// clang-format off
REGISTER_LARA_STATE(LS_SLIDE,        M_Slide)
REGISTER_LARA_STATE(LS_SLIDE_BACK,   M_SlideBack)
REGISTER_LARA_STATE(LS_PUSH_BLOCK,   M_PushBlock)
REGISTER_LARA_STATE(LS_PULL_BLOCK,   M_PushBlock)
REGISTER_LARA_STATE(LS_PP_READY,     M_PPReady)
REGISTER_LARA_STATE(LS_PICKUP,       M_Pickup)
REGISTER_LARA_STATE(LS_SWITCH_ON,    M_SwitchOn)
REGISTER_LARA_STATE(LS_SWITCH_OFF,   M_SwitchOn)
REGISTER_LARA_STATE(LS_USE_KEY,      M_UseKey)
REGISTER_LARA_STATE(LS_USE_PUZZLE,   M_UseKey)
REGISTER_LARA_STATE(LS_SPECIAL,      M_Special)
REGISTER_LARA_STATE(LS_FLARE_PICKUP, M_PickupFlare)
// clang-format on
