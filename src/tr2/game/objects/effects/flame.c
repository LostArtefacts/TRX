#include "game/collide.h"
#include "game/effects.h"
#include "game/lara/control.h"
#include "game/lara/misc.h"
#include "game/room.h"
#include "game/sound.h"
#include "global/types.h"
#include "global/vars.h"

#include <libtrx/utils.h>

static void M_Setup(OBJECT *obj);
static void M_Control(int16_t effect_num);

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->semi_transparent = 1;
}
static void M_Control(const int16_t effect_num)
{
    EFFECT *const effect = Effect_Get(effect_num);

    effect->frame_num--;
    if (effect->frame_num <= Object_Get(O_FLAME)->mesh_count) {
        effect->frame_num = 0;
    }

    if (effect->counter >= 0) {
        Sound_Effect(SFX_LOOP_FOR_SMALL_FIRES, &effect->pos, SPM_ALWAYS);
        if (effect->counter != 0) {
            effect->counter--;
        } else if (Lara_IsNearItem(&effect->pos, 600)) {
            Lara_TakeDamage(5, true);
            const int32_t dx = g_LaraItem->pos.x - effect->pos.x;
            const int32_t dz = g_LaraItem->pos.z - effect->pos.z;
            const int32_t dist = SQUARE(dx) + SQUARE(dz);
            if (dist < SQUARE(450)) {
                effect->counter = 100;
                Lara_CatchFire();
            }
        }
    } else {
        effect->pos.x = 0;
        effect->pos.y = 0;
        if (effect->counter == -1) {
            effect->pos.z = -100;
        } else {
            effect->pos.z = 0;
        }

        Collide_GetJointAbsPosition(
            g_LaraItem, &effect->pos, -1 - effect->counter);
        const int16_t room_num = g_LaraItem->room_num;
        if (room_num != effect->room_num) {
            Effect_NewRoom(effect_num, room_num);
        }

        const int32_t water_height = Room_GetWaterHeight(
            effect->pos.x, effect->pos.y, effect->pos.z, effect->room_num);
        if ((water_height != NO_HEIGHT && effect->pos.y > water_height)
            || g_Lara.water_status == LWS_CHEAT) {
            effect->counter = 0;
            Effect_Kill(effect_num);
            g_Lara.burn = 0;
        } else {
            Sound_Effect(SFX_LOOP_FOR_SMALL_FIRES, &effect->pos, SPM_ALWAYS);
            Lara_TakeDamage(7, false);
            g_Lara.burn = 1;
        }
    }
}

REGISTER_OBJECT(O_FLAME, M_Setup)
