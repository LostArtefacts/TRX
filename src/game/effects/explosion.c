#include "game/effects/explosion.h"

#include "game/items.h"
#include "game/sound.h"
#include "global/vars.h"

void SetupExplosion(OBJECT_INFO *obj)
{
    obj->control = ControlExplosion1;
}

void ControlExplosion1(int16_t fx_num)
{
    FX_INFO *fx = &Effects[fx_num];
    fx->counter++;
    if (fx->counter == 2) {
        fx->counter = 0;
        fx->frame_number--;
        if (fx->frame_number <= Objects[fx->object_number].nmeshes) {
            KillEffect(fx_num);
        }
    }
}

void Explosion(ITEM_INFO *item)
{
    Sound_Effect(SFX_EXPLOSION_FX, NULL, SPM_NORMAL);
    Camera.bounce = -75;
    FlipEffect = -1;
}
