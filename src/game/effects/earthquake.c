#include "game/effects/earthquake.h"

#include "game/control.h"
#include "game/draw.h"
#include "game/game.h"
#include "game/sound.h"
#include "global/vars.h"

void SetupEarthquake(OBJECT_INFO *obj)
{
    obj->control = EarthQuakeControl;
    obj->draw_routine = DrawDummyItem;
    obj->save_flags = 1;
}

// original name: EarthQuakeFX
void EarthQuake(ITEM_INFO *item)
{
    if (FlipTimer == 0) {
        SoundEffect(SFX_EXPLOSION, NULL, SPM_NORMAL);
        Camera.bounce = -250;
    } else if (FlipTimer == 3) {
        SoundEffect(SFX_ROLLING_BALL, NULL, SPM_NORMAL);
    } else if (FlipTimer == 35) {
        SoundEffect(SFX_EXPLOSION, NULL, SPM_NORMAL);
    } else if (FlipTimer == 20 || FlipTimer == 50 || FlipTimer == 70) {
        SoundEffect(SFX_T_REX_FOOTSTOMP, NULL, SPM_NORMAL);
    }

    FlipTimer++;
    if (FlipTimer == 105) {
        FlipEffect = -1;
    }
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
