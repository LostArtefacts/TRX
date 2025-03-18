#include "game/effects.h"
#include "global/vars.h"

static void M_Setup(OBJECT *obj);
static void M_Control(int16_t effect_num);

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
}

static void M_Control(const int16_t effect_num)
{
    EFFECT *const effect = Effect_Get(effect_num);
    effect->counter--;
    if (effect->counter == 0) {
        Effect_Kill(effect_num);
    }
}

REGISTER_OBJECT(O_RICOCHET, M_Setup)
