#include "game/control.h"
#include "game/effects.h"
#include "game/game.h"
#include "game/items.h"
#include "game/sphere.h"
#include "game/traps.h"
#include "game/vars.h"
#include "util.h"

void FlameControl(int16_t fx_num)
{
    FX_INFO* fx = &Effects[fx_num];

    fx->frame_number--;
    if (fx->frame_number <= Objects[O_FLAME].nmeshes) {
        fx->frame_number = 0;
    }

    if (fx->counter < 0) {
#ifdef T1M_FEAT_CHEATS
        if (Lara.water_status == LWS_CHEAT) {
            fx->counter = 0;
            StopSoundEffect(150, NULL);
            KillEffect(fx_num);
        }
#endif

        fx->pos.x = 0;
        fx->pos.y = 0;
        if (fx->counter == -1) {
            fx->pos.z = -100;
        } else {
            fx->pos.z = 0;
        }

        GetJointAbsPosition(LaraItem, (PHD_VECTOR*)&fx->pos, -1 - fx->counter);

#ifdef T1M_FEAT_OG_FIXES
        int32_t y = GetWaterHeight(
            LaraItem->pos.x, LaraItem->pos.y, LaraItem->pos.z,
            LaraItem->room_number);
#else
        int32_t y =
            GetWaterHeight(fx->pos.x, fx->pos.y, fx->pos.z, fx->room_number);
#endif

        if (y != NO_HEIGHT && fx->pos.y > y) {
            fx->counter = 0;
            StopSoundEffect(150, NULL);
            KillEffect(fx_num);
        } else {
            SoundEffect(150, &fx->pos, 0);
            LaraItem->hit_points -= FLAME_ONFIRE_DAMAGE;
            LaraItem->hit_status = 1;
        }
        return;
    }

    SoundEffect(150, &fx->pos, 0);
    if (fx->counter) {
        fx->counter--;
    } else if (ItemNearLara(&fx->pos, 600)) {
#ifdef T1M_FEAT_CHEATS
        if (Lara.water_status == LWS_CHEAT) {
            return;
        }
#endif

        int32_t x = LaraItem->pos.x - fx->pos.x;
        int32_t z = LaraItem->pos.z - fx->pos.z;
        int32_t distance = SQUARE(x) + SQUARE(z);

        LaraItem->hit_points -= FLAME_TOONEAR_DAMAGE;
        LaraItem->hit_status = 1;

        if (distance < SQUARE(300)) {
            fx->counter = 100;

            fx_num = CreateEffect(LaraItem->room_number);
            if (fx_num != -1) {
                fx = &Effects[fx_num];
                fx->frame_number = 0;
                fx->object_number = O_FLAME;
                fx->counter = -1;
            }
        }
    }
}

void LavaBurn(ITEM_INFO* item)
{
#ifdef T1M_FEAT_CHEATS
    if (Lara.water_status == LWS_CHEAT) {
        return;
    }
#endif

    if (item->hit_points < 0) {
        return;
    }

    int16_t room_num = item->room_number;
    FLOOR_INFO* floor = GetFloor(item->pos.x, 32000, item->pos.z, &room_num);

    if (item->floor != GetHeight(floor, item->pos.x, 32000, item->pos.z)) {
        return;
    }

    item->hit_points = -1;
    item->hit_status = 1;
    for (int i = 0; i < 10; i++) {
        int16_t fx_num = CreateEffect(item->room_number);
        if (fx_num != NO_ITEM) {
            FX_INFO* fx = &Effects[fx_num];
            fx->object_number = O_FLAME;
            fx->frame_number =
                (Objects[O_FLAME].nmeshes * GetRandomControl()) / 0x7FFF;
            fx->counter = -1 - GetRandomControl() * 24 / 0x7FFF;
        }
    }
}

void T1MInjectGameTraps()
{
    INJECT(0x0043B2A0, FlameControl);
    INJECT(0x0043B430, LavaBurn);
}
