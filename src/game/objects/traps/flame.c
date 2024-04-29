#include "game/objects/traps/flame.h"

#include "game/collide.h"
#include "game/effects.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/objects/common.h"
#include "game/room.h"
#include "game/sound.h"
#include "global/const.h"
#include "global/vars.h"

#include <libtrx/utils.h>

#include <stdbool.h>
#include <stddef.h>

#define FLAME_ONFIRE_DAMAGE 5
#define FLAME_TOONEAR_DAMAGE 3

void Flame_Setup(OBJECT_INFO *obj)
{
    obj->control = Flame_Control;
}

void Flame_Control(int16_t fx_num)
{
    FX_INFO *fx = &g_Effects[fx_num];

    fx->frame_number--;
    if (fx->frame_number <= g_Objects[O_FLAME].nmeshes) {
        fx->frame_number = 0;
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
            g_LaraItem->room_number);

        if (y != NO_HEIGHT && fx->pos.y > y) {
            fx->counter = 0;
            Sound_StopEffect(SFX_FIRE, NULL);
            Effect_Kill(fx_num);
        } else {
            if (fx->room_number != g_LaraItem->room_number) {
                Effect_NewRoom(fx_num, g_LaraItem->room_number);
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

            fx_num = Effect_Create(g_LaraItem->room_number);
            if (fx_num != NO_ITEM) {
                fx = &g_Effects[fx_num];
                fx->frame_number = 0;
                fx->object_number = O_FLAME;
                fx->counter = -1;
            }
        }
    }
}

void FlameEmitter_Setup(OBJECT_INFO *obj)
{
    obj->control = FlameEmitter_Control;
    obj->draw_routine = Object_DrawDummyItem;
    obj->save_flags = 1;
}

void FlameEmitter_Control(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];
    if (Item_IsTriggerActive(item)) {
        if (!item->data) {
            int16_t fx_num = Effect_Create(item->room_number);
            if (fx_num != NO_ITEM) {
                FX_INFO *fx = &g_Effects[fx_num];
                fx->pos.x = item->pos.x;
                fx->pos.y = item->pos.y;
                fx->pos.z = item->pos.z;
                fx->frame_number = 0;
                fx->object_number = O_FLAME;
                fx->counter = 0;
            }
            item->data = (void *)(intptr_t)(fx_num + 1);
        }
    } else if (item->data) {
        Sound_StopEffect(SFX_FIRE, NULL);
        Effect_Kill((int16_t)(intptr_t)item->data - 1);
        item->data = NULL;
    }
}
