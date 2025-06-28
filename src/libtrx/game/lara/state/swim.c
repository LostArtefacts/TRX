#include "config.h"
#include "game/camera.h"
#include "game/input.h"
#include "game/lara.h"
#include "game/lara/util.h"

static void M_SurfTread(ITEM *item, COLL_INFO *coll);
static void M_ForwardSurface(ITEM *item, COLL_INFO *coll);
static void M_SideBackSurface(ITEM *item, COLL_INFO *coll);
static void M_Dive(ITEM *item, COLL_INFO *coll);
static void M_UWDeath(ITEM *item, COLL_INFO *coll);
static void M_WaterOut(ITEM *item, COLL_INFO *coll);
static void M_UWTwist(ITEM *item, COLL_INFO *coll);

static void M_SurfTread(ITEM *const item, COLL_INFO *const coll)
{
    item->fall_speed -= 4;
    CLAMPL(item->fall_speed, 0);

    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_UW_DEATH;
        return;
    }

#if TR_VERSION == 1
    coll->enable_hit = 0;
    if (g_Input.look) {
        Lara_LookLeftRightSurf();
        Lara_LookUpDownSurf();
        return;
    }
    if (g_Camera.type == CAM_LOOK) {
        g_Camera.type = CAM_CHASE;
    }
#else
    if (g_Input.look) {
        Lara_LookUpDown();
        return;
    }
#endif

    if (g_Input.left) {
        item->rot.y -= LARA_SLOW_TURN;
    } else if (g_Input.right) {
        item->rot.y += LARA_SLOW_TURN;
    }

    if (g_Input.forward) {
        item->goal_anim_state = LS_SURF_SWIM;
    } else if (g_Input.back) {
        item->goal_anim_state = LS_SURF_BACK;
    }

    if (g_Input.step_left) {
        item->goal_anim_state = LS_SURF_LEFT;
    } else if (g_Input.step_right) {
        item->goal_anim_state = LS_SURF_RIGHT;
    }

    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (g_Input.jump) {
        lara->dive_timer++;
        if (lara->dive_timer == LARA_DIVE_WAIT) {
            Item_SwitchToAnim(item, LA_ONWATER_DIVE, 0);
            item->goal_anim_state = LS_SWIM;
            item->current_anim_state = LS_DIVE;
            item->rot.x = -45 * DEG_1;
            item->fall_speed = 80;
            lara->water_status = LWS_UNDERWATER;
        }
    } else {
        lara->dive_timer = 0;
    }
}

static void M_ForwardSurface(ITEM *const item, COLL_INFO *const coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_UW_DEATH;
        return;
    }

#if TR_VERSION == 1
    coll->enable_hit = 0;
#endif
    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->dive_timer = 0;
    if (!g_Config.input.enable_tr3_sidesteps || !g_Input.slow) {
        if (g_Input.left) {
            item->rot.y -= LARA_SLOW_TURN;
        } else if (g_Input.right) {
            item->rot.y += LARA_SLOW_TURN;
        }
    }
    if (!g_Input.forward || g_Input.jump) {
        item->goal_anim_state = LS_SURF_TREAD;
    }
    item->fall_speed += 8;
    CLAMPG(item->fall_speed, LARA_MAX_SURF_SPEED);
}

static void M_SideBackSurface(ITEM *const item, COLL_INFO *const coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_UW_DEATH;
        return;
    }

#if TR_VERSION == 1
    coll->enable_hit = 0;
#endif
    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->dive_timer = 0;

    if (!g_Config.input.enable_tr3_sidesteps || !g_Input.slow) {
        if (g_Input.left) {
            item->rot.y -= LARA_SURF_TURN;
        } else if (g_Input.right) {
            item->rot.y += LARA_SURF_TURN;
        }

        bool stop = false;
        switch (item->current_anim_state) {
        case LS_SURF_BACK:
            stop = !g_Input.back;
            break;
        case LS_SURF_LEFT:
            stop = !g_Input.step_left;
            break;
        case LS_SURF_RIGHT:
            stop = !g_Input.step_right;
            break;
        default:
            break;
        }

        if (stop) {
            item->goal_anim_state = LS_SURF_TREAD;
        }
    }

    item->fall_speed += 8;
    CLAMPG(item->fall_speed, LARA_MAX_SURF_SPEED);
}

static void M_Dive(ITEM *const item, COLL_INFO *const coll)
{
    if (g_Input.forward) {
        item->rot.x -= DEG_1;
    }
}

static void M_UWDeath(ITEM *const item, COLL_INFO *const coll)
{
#if TR_VERSION == 1
    coll->enable_hit = 0;
#endif
    item->gravity = false;
    item->fall_speed -= 8;
    CLAMPL(item->fall_speed, 0);

    const int32_t angle = 2 * DEG_1;
    if (item->rot.x >= -angle && item->rot.x <= angle) {
        item->rot.x = 0;
    } else if (item->rot.x >= 0) {
        item->rot.x -= angle;
    } else {
        item->rot.x += angle;
    }
}

static void M_WaterOut(ITEM *const item, COLL_INFO *const coll)
{
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
    g_Camera.flags = CF_FOLLOW_CENTRE;
}

static void M_UWTwist(ITEM *const item, COLL_INFO *const coll)
{
    item->fall_speed = 0;
    item->goal_anim_state = LS_TREAD;
}

// clang-format off
REGISTER_LARA_STATE(LS_SURF_TREAD, M_SurfTread)
REGISTER_LARA_STATE(LS_SURF_SWIM,  M_ForwardSurface)
REGISTER_LARA_STATE(LS_DIVE,       M_Dive)
REGISTER_LARA_STATE(LS_UW_DEATH,   M_UWDeath)
REGISTER_LARA_STATE(LS_SURF_BACK,  M_SideBackSurface)
REGISTER_LARA_STATE(LS_SURF_LEFT,  M_SideBackSurface)
REGISTER_LARA_STATE(LS_SURF_RIGHT, M_SideBackSurface)
REGISTER_LARA_STATE(LS_WATER_OUT,  M_WaterOut)
REGISTER_LARA_STATE(LS_WATER_ROLL, M_UWTwist)
// clang-format on
