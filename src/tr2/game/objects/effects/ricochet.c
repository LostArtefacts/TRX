#include "game/objects/effects/ricochet.h"

#include "game/effects.h"
#include "global/vars.h"

void Spawn_Ricochet_Control(const int16_t effect_num)
{
    EFFECT *const effect = Effect_Get(effect_num);
    effect->counter--;
    if (effect->counter == 0) {
        Effect_Kill(effect_num);
    }
}

void Spawn_Ricochet_Setup(void)
{
    OBJECT *const obj = Object_Get(O_RICOCHET);
    obj->control_func = Spawn_Ricochet_Control;
}
