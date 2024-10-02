#include "game/objects/effects/flame.h"

#include "game/collide.h"
#include "game/effects.h"
#include "game/lara/common.h"
#include "game/lara/control.h"
#include "game/room.h"
#include "game/sound.h"
#include "global/vars.h"

#define FLAME_ONFIRE_DAMAGE 5
#define FLAME_TOONEAR_DAMAGE 3

void Flame_Setup(OBJECT *obj)
{
    obj->control = Flame_Control;
}

void Flame_Control(int16_t fx_num)
{
    FX *fx = &g_Effects[fx_num];

    fx->frame_num--;
    if (fx->frame_num <= g_Objects[O_FLAME].nmeshes) {
        fx->frame_num = 0;
    }

    if (fx->counter < 0) {
        if (g_Lara.water_status == LWS_CHEAT) {
            fx->counter = 0;
            Sound_StopEffect(SFX_FIRE, NULL);
            Effect_Kill(fx_num);
        }

        fx->pos.x = 0;
        fx->pos.y = 0;
        if (fx->counter == -1) {
            fx->pos.z = -100;
        } else {
            fx->pos.z = 0;
        }

        Collide_GetJointAbsPosition(g_LaraItem, &fx->pos, -1 - fx->counter);

        int32_t y = Room_GetWaterHeight(
            g_LaraItem->pos.x, g_LaraItem->pos.y, g_LaraItem->pos.z,
            g_LaraItem->room_num);

        if (y != NO_HEIGHT && fx->pos.y > y) {
            fx->counter = 0;
            Sound_StopEffect(SFX_FIRE, NULL);
            Effect_Kill(fx_num);
        } else {
            if (fx->room_num != g_LaraItem->room_num) {
                Effect_NewRoom(fx_num, g_LaraItem->room_num);
            }
            Sound_Effect(SFX_FIRE, &fx->pos, SPM_NORMAL);
            Lara_TakeDamage(FLAME_ONFIRE_DAMAGE, true);
        }
        return;
    }

    Sound_Effect(SFX_FIRE, &fx->pos, SPM_NORMAL);
    if (fx->counter) {
        fx->counter--;
    } else if (Lara_IsNearItem(&fx->pos, 600)) {
        if (g_Lara.water_status == LWS_CHEAT) {
            return;
        }

        int32_t x = g_LaraItem->pos.x - fx->pos.x;
        int32_t z = g_LaraItem->pos.z - fx->pos.z;
        int32_t distance = SQUARE(x) + SQUARE(z);

        Lara_TakeDamage(FLAME_TOONEAR_DAMAGE, true);

        if (distance < SQUARE(300)) {
            fx->counter = 100;

            fx_num = Effect_Create(g_LaraItem->room_num);
            if (fx_num != NO_ITEM) {
                fx = &g_Effects[fx_num];
                fx->frame_num = 0;
                fx->object_id = O_FLAME;
                fx->counter = -1;
            }
        }
    }
}
