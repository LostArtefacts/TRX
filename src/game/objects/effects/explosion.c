#include "game/objects/effects/explosion.h"

#include "game/items.h"
#include "global/vars.h"

void Explosion_Setup(OBJECT_INFO *obj)
{
    obj->control = Explosion_Control;
}

void Explosion_Control(int16_t fx_num)
{
    FX_INFO *fx = &g_Effects[fx_num];
    fx->counter++;
    if (fx->counter == 2) {
        fx->counter = 0;
        fx->frame_number--;
        if (fx->frame_number <= g_Objects[fx->object_number].nmeshes) {
            Effect_Kill(fx_num);
        }
    }
}
