#include "config.h"
#include "game/input.h"
#include "game/lara.h"
#include "game/lara/util.h"

static void M_SurfSwim(ITEM *item, COLL_INFO *coll);

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

// clang-format off
REGISTER_LARA_STATE(LS_SURF_SWIM,  M_SurfSwim)
// clang-format on
