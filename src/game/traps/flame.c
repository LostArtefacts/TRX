#include "game/control.h"
#include "game/draw.h"
#include "game/effects.h"
#include "game/items.h"
#include "game/sphere.h"
#include "game/traps/flame.h"
#include "game/vars.h"

void SetupFlameEmitter(OBJECT_INFO *obj)
{
    obj->control = FlameEmitterControl;
    obj->draw_routine = DrawDummyItem;
}

void SetupFlame(OBJECT_INFO *obj)
{
    obj->control = FlameControl;
}

void FlameEmitterControl(int16_t item_num)
{
    ITEM_INFO *item = &Items[item_num];
    if (TriggerActive(item)) {
        if (!item->data) {
            int16_t fx_num = CreateEffect(item->room_number);
            if (fx_num != NO_ITEM) {
                FX_INFO *fx = &Effects[fx_num];
                fx->pos.x = item->pos.x;
                fx->pos.y = item->pos.y;
                fx->pos.z = item->pos.z;
                fx->frame_number = 0;
                fx->object_number = O_FLAME;
                fx->counter = 0;
            }
            item->data = (void *)(fx_num + 1);
        }
    } else if (item->data) {
        StopSoundEffect(SFX_FIRE, NULL);
        KillEffect((int16_t)(size_t)item->data - 1);
        item->data = NULL;
    }
}

void FlameControl(int16_t fx_num)
{
    FX_INFO *fx = &Effects[fx_num];

    fx->frame_number--;
    if (fx->frame_number <= Objects[O_FLAME].nmeshes) {
        fx->frame_number = 0;
    }

    if (fx->counter < 0) {
        if (Lara.water_status == LWS_CHEAT) {
            fx->counter = 0;
            StopSoundEffect(SFX_FIRE, NULL);
            KillEffect(fx_num);
        }

        fx->pos.x = 0;
        fx->pos.y = 0;
        if (fx->counter == -1) {
            fx->pos.z = -100;
        } else {
            fx->pos.z = 0;
        }

        GetJointAbsPosition(LaraItem, (PHD_VECTOR *)&fx->pos, -1 - fx->counter);

        int32_t y = GetWaterHeight(
            LaraItem->pos.x, LaraItem->pos.y, LaraItem->pos.z,
            LaraItem->room_number);

        if (y != NO_HEIGHT && fx->pos.y > y) {
            fx->counter = 0;
            StopSoundEffect(SFX_FIRE, NULL);
            KillEffect(fx_num);
        } else {
            SoundEffect(SFX_FIRE, &fx->pos, SPM_NORMAL);
            LaraItem->hit_points -= FLAME_ONFIRE_DAMAGE;
            LaraItem->hit_status = 1;
        }
        return;
    }

    SoundEffect(SFX_FIRE, &fx->pos, SPM_NORMAL);
    if (fx->counter) {
        fx->counter--;
    } else if (ItemNearLara(&fx->pos, 600)) {
        if (Lara.water_status == LWS_CHEAT) {
            return;
        }

        int32_t x = LaraItem->pos.x - fx->pos.x;
        int32_t z = LaraItem->pos.z - fx->pos.z;
        int32_t distance = SQUARE(x) + SQUARE(z);

        LaraItem->hit_points -= FLAME_TOONEAR_DAMAGE;
        LaraItem->hit_status = 1;

        if (distance < SQUARE(300)) {
            fx->counter = 100;

            fx_num = CreateEffect(LaraItem->room_number);
            if (fx_num != NO_ITEM) {
                fx = &Effects[fx_num];
                fx->frame_number = 0;
                fx->object_number = O_FLAME;
                fx->counter = -1;
            }
        }
    }
}
