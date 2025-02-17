#include "game/objects/effects/ricochet.h"

#include "game/effects.h"

void Ricochet_Setup(OBJECT *obj)
{
    obj->control_func = Ricochet_Control;
}

void Ricochet_Control(int16_t effect_num)
{
    EFFECT *const effect = Effect_Get(effect_num);
    effect->counter--;
    if (effect->counter == 0) {
        Effect_Kill(effect_num);
    }
}
