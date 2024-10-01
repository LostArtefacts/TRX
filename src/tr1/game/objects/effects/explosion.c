#include "game/objects/effects/explosion.h"

#include "game/effects.h"
#include "global/vars.h"

void Explosion_Setup(OBJECT *obj)
{
    obj->control = Explosion_Control;
}

void Explosion_Control(int16_t fx_num)
{
    FX *fx = &g_Effects[fx_num];
    fx->counter++;
    if (fx->counter == 2) {
        fx->counter = 0;
        fx->frame_num--;
        if (fx->frame_num <= g_Objects[fx->object_id].nmeshes) {
            Effect_Kill(fx_num);
        }
    }
}
