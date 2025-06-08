#include "game/input.h"
#include "game/lara/common.h"
#include "game/lara/misc.h"
#include "game/sound.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/game/collision.h>
#include <libtrx/game/lara.h>

#include <stdint.h>

void Lara_Col_Hang(ITEM *item, COLL_INFO *coll)
{
    Lara_HangTest(item, coll);
    if (item->goal_anim_state == LS_HANG && g_Input.forward) {
        if (coll->side_front.floor > -850 && coll->side_front.floor < -650
            && coll->side_front.floor - coll->side_front.ceiling >= 0
            && coll->side_left.floor - coll->side_left.ceiling >= 0
            && coll->side_right.floor - coll->side_right.ceiling >= 0
            && !coll->hit_static) {
            item->goal_anim_state = g_Input.slow ? LS_GYMNAST : LS_PULL_UP;
        }
    }
}
