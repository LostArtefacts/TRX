#include "config.h"
#include "game/camera.h"
#include "game/input.h"
#include "game/lara.h"
#include "game/lara/util.h"

static void M_SurfSwim(ITEM *item, COLL_INFO *coll);
static void M_Dive(ITEM *item, COLL_INFO *coll);
static void M_WaterOut(ITEM *item, COLL_INFO *coll);

static void M_SurfSwim(ITEM *const item, COLL_INFO *const coll)
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

static void M_Dive(ITEM *const item, COLL_INFO *const coll)
{
    if (g_Input.forward) {
        item->rot.x -= DEG_1;
    }
}

static void M_WaterOut(ITEM *const item, COLL_INFO *const coll)
{
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
    g_Camera.flags = CF_FOLLOW_CENTRE;
}

// clang-format off
REGISTER_LARA_STATE(LS_SURF_SWIM,  M_SurfSwim)
REGISTER_LARA_STATE(LS_DIVE,       M_Dive)
REGISTER_LARA_STATE(LS_WATER_OUT,  M_WaterOut)
// clang-format on
