#include <trx/game/input.h>
#include <trx/game/lara.h>
#include <trx/game/lara/util.h>

#define M_TURN_RATE 256

static void M_PoleSpin(ITEM *const item, COLL_INFO *const coll)
{
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;

    const bool turning = item->current_anim_state == LS(LS_POLE_LEFT)
        ? g_Input.left
        : g_Input.right;
    if (!turning || !g_Input.action || g_Input.forward || g_Input.back
        || item->hit_points <= 0) {
        item->goal_anim_state = LS(LS_POLE_IDLE);
    } else if (item->current_anim_state == LS(LS_POLE_LEFT)) {
        item->rot.y += M_TURN_RATE;
    } else {
        item->rot.y -= M_TURN_RATE;
    }
}

REGISTER_LARA_STATE(LS_POLE_LEFT, M_PoleSpin)
REGISTER_LARA_STATE(LS_POLE_RIGHT, M_PoleSpin)
