#include "game/objects/effects/flame.h"

#include "game/collide.h"
#include "game/effects.h"
#include "game/lara/control.h"
#include "game/lara/misc.h"
#include "game/room.h"
#include "game/sound.h"
#include "global/funcs.h"
#include "global/types.h"
#include "global/vars.h"

#include <libtrx/utils.h>

void __cdecl Flame_Control(const int16_t fx_num)
{
    FX *const fx = &g_Effects[fx_num];

    fx->frame_num--;
    if (fx->frame_num <= g_Objects[O_FLAME].mesh_count) {
        fx->frame_num = 0;
    }

    if (fx->counter >= 0) {
        Sound_Effect(SFX_LOOP_FOR_SMALL_FIRES, &fx->pos, SPM_ALWAYS);
        if (fx->counter != 0) {
            fx->counter--;
        } else if (Lara_IsNearItem(&fx->pos, 600)) {
            Lara_TakeDamage(5, true);
            const int32_t dx = g_LaraItem->pos.x - fx->pos.x;
            const int32_t dz = g_LaraItem->pos.z - fx->pos.z;
            const int32_t dist = SQUARE(dx) + SQUARE(dz);
            if (dist < SQUARE(450)) {
                fx->counter = 100;
                Lara_CatchFire();
            }
        }
    } else {
        fx->pos.x = 0;
        fx->pos.y = 0;
        if (fx->counter == -1) {
            fx->pos.z = -100;
        } else {
            fx->pos.z = 0;
        }

        Collide_GetJointAbsPosition(g_LaraItem, &fx->pos, -1 - fx->counter);
        const int16_t room_num = g_LaraItem->room_num;
        if (room_num != fx->room_num) {
            Effect_NewRoom(fx_num, room_num);
        }

        const int32_t water_height =
            Room_GetWaterHeight(fx->pos.x, fx->pos.y, fx->pos.z, fx->room_num);
        if ((water_height != NO_HEIGHT && fx->pos.y > water_height)
            || g_Lara.water_status == LWS_CHEAT) {
            fx->counter = 0;
            Effect_Kill(fx_num);
            g_Lara.burn = 0;
        } else {
            Sound_Effect(SFX_LOOP_FOR_SMALL_FIRES, &fx->pos, SPM_ALWAYS);
            Lara_TakeDamage(7, false);
            g_Lara.burn = 1;
        }
    }
}
