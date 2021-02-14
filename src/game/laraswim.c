#include "game/collide.h"
#include "game/const.h"
#include "game/control.h"
#include "game/data.h"
#include "game/lara.h"
#include "game/shell.h"
#include "mod.h"

void __cdecl LaraUnderWater(ITEM_INFO* item, COLL_INFO* coll)
{
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -UW_HITE;
    coll->bad_ceiling = UW_HITE;
    coll->old.x = item->pos.x;
    coll->old.y = item->pos.y;
    coll->old.z = item->pos.z;
    coll->radius = UW_RADIUS;
    coll->trigger = NULL;
    coll->slopes_are_walls = 0;
    coll->slopes_are_pits = 0;
    coll->lava_is_pit = 0;
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;

    if (TR1MConfig.enable_enhanced_look) {
        if (Input & IN_LOOK) {
            TR1MLookLeftRight();
        } else {
            TR1MResetLook();
        }
    }

    LaraControlRoutines[item->current_anim_state](item, coll);

    if (item->pos.z_rot >= -(2 * LARA_LEAN_UNDO)
        && item->pos.z_rot <= 2 * LARA_LEAN_UNDO) {
        item->pos.z_rot = 0;
    } else if (item->pos.z_rot < 0) {
        item->pos.z_rot += 2 * LARA_LEAN_UNDO;
    } else {
        item->pos.z_rot -= 2 * LARA_LEAN_UNDO;
    }

    if (item->pos.x_rot < -100 * ONE_DEGREE) {
        item->pos.x_rot = -100 * ONE_DEGREE;
    } else if (item->pos.x_rot > 100 * ONE_DEGREE) {
        item->pos.x_rot = 100 * ONE_DEGREE;
    }

    if (item->pos.z_rot < -LARA_LEAN_MAX_UW) {
        item->pos.z_rot = -LARA_LEAN_MAX_UW;
    } else if (item->pos.z_rot > LARA_LEAN_MAX_UW) {
        item->pos.z_rot = LARA_LEAN_MAX_UW;
    }

    if (Lara.current_active && Lara.water_status != LARA_CHEAT) {
        LaraWaterCurrent(coll);
    }

    AnimateLara(item);
    item->pos.y -=
        (phd_sin(item->pos.x_rot) * item->fall_speed) >> (W2V_SHIFT + 2);

    item->pos.x +=
        (((phd_sin(item->pos.y_rot) * item->fall_speed) >> (W2V_SHIFT + 2))
         * phd_cos(item->pos.x_rot))
        >> W2V_SHIFT;

    item->pos.z +=
        (((phd_cos(item->pos.y_rot) * item->fall_speed) >> (W2V_SHIFT + 2))
         * phd_cos(item->pos.x_rot))
        >> W2V_SHIFT;

    if (Lara.water_status != LARA_CHEAT) {
        LaraBaddieCollision(item, coll);
    }

    LaraCollisionRoutines[item->current_anim_state](item, coll);
    UpdateLaraRoom(item, 0);
    LaraGun();
    TestTriggers(coll->trigger, 0);
}

void TR1MInjectLaraSwim()
{
    INJECT(0x00428F10, LaraUnderWater);
}
