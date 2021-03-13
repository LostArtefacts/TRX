#include "game/control.h"
#include "game/draw.h"
#include "game/effects.h"
#include "game/game.h"
#include "game/objects/earthquake.h"
#include "game/vars.h"

void SetupEarthquake(OBJECT_INFO *obj)
{
    obj->control = EarthQuakeControl;
    obj->draw_routine = DrawDummyItem;
    obj->save_flags = 1;
}

// original name: EarthQuake
void EarthQuakeControl(int16_t item_num)
{
    ITEM_INFO *item = &Items[item_num];
    if (TriggerActive(item)) {
        if (GetRandomDraw() < 0x100) {
            Camera.bounce = -150;
            SoundEffect(SFX_ROLLING_BALL, NULL, SPM_NORMAL);
        } else if (GetRandomControl() < 0x400) {
            Camera.bounce = 50;
            SoundEffect(SFX_T_REX_FOOTSTOMP, NULL, SPM_NORMAL);
        }
    }
}
