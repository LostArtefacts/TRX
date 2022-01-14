#include "game/effects/earthquake.h"

#include "game/control.h"
#include "game/draw.h"
#include "game/random.h"
#include "game/sound.h"
#include "global/vars.h"

void SetupEarthquake(OBJECT_INFO *obj)
{
    obj->control = EarthQuakeControl;
    obj->draw_routine = DrawDummyItem;
    obj->save_flags = 1;
}

void EarthQuake(ITEM_INFO *item)
{
    if (g_FlipTimer == 0) {
        Sound_Effect(SFX_EXPLOSION, NULL, SPM_NORMAL);
        g_Camera.bounce = -250;
    } else if (g_FlipTimer == 3) {
        Sound_Effect(SFX_ROLLING_BALL, NULL, SPM_NORMAL);
    } else if (g_FlipTimer == 35) {
        Sound_Effect(SFX_EXPLOSION, NULL, SPM_NORMAL);
    } else if (g_FlipTimer == 20 || g_FlipTimer == 50 || g_FlipTimer == 70) {
        Sound_Effect(SFX_T_REX_FOOTSTOMP, NULL, SPM_NORMAL);
    }

    g_FlipTimer++;
    if (g_FlipTimer == 105) {
        g_FlipEffect = -1;
    }
}

void EarthQuakeControl(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];
    if (TriggerActive(item)) {
        if (Random_GetDraw() < 0x100) {
            g_Camera.bounce = -150;
            Sound_Effect(SFX_ROLLING_BALL, NULL, SPM_NORMAL);
        } else if (Random_GetControl() < 0x400) {
            g_Camera.bounce = 50;
            Sound_Effect(SFX_T_REX_FOOTSTOMP, NULL, SPM_NORMAL);
        }
    }
}
