#include "game/effects/splash.h"

#include "3dsystem/phd_math.h"
#include "game/control.h"
#include "game/game.h"
#include "game/items.h"
#include "game/sound.h"
#include "global/vars.h"

void SetupSplash(OBJECT_INFO *obj)
{
    obj->control = ControlSplash1;
}

void Splash(ITEM_INFO *item)
{
    int16_t wh = GetWaterHeight(
        item->pos.x, item->pos.y, item->pos.z, item->room_number);
    int16_t room_num = item->room_number;
    GetFloor(item->pos.x, item->pos.y, item->pos.z, &room_num);

    Sound_Effect(SFX_LARA_SPLASH, &item->pos, SPM_NORMAL);

    for (int i = 0; i < 10; i++) {
        int16_t fx_num = CreateEffect(room_num);
        if (fx_num != NO_ITEM) {
            FX_INFO *fx = &Effects[fx_num];
            fx->pos.x = item->pos.x;
            fx->pos.y = wh;
            fx->pos.z = item->pos.z;
            fx->pos.y_rot = PHD_180 + 2 * GetRandomDraw();
            fx->object_number = O_SPLASH1;
            fx->frame_number = 0;
            fx->speed = GetRandomDraw() / 256;
        }
    }
}

void ControlSplash1(int16_t fx_num)
{
    FX_INFO *fx = &Effects[fx_num];
    fx->frame_number--;
    if (fx->frame_number <= Objects[fx->object_number].nmeshes) {
        KillEffect(fx_num);
        return;
    }

    fx->pos.z += (phd_cos(fx->pos.y_rot) * fx->speed) >> W2V_SHIFT;
    fx->pos.x += (phd_sin(fx->pos.y_rot) * fx->speed) >> W2V_SHIFT;
}
