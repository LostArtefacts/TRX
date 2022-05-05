#include "game/objects/effects/blood.h"

#include "game/effects.h"
#include "global/vars.h"
#include "math/math.h"

void Blood_Setup(OBJECT_INFO *obj)
{
    obj->control = Blood_Control;
}

void Blood_Control(int16_t fx_num)
{
    FX_INFO *fx = &g_Effects[fx_num];
    fx->pos.x += (Math_Sin(fx->pos.y_rot) * fx->speed) >> W2V_SHIFT;
    fx->pos.z += (Math_Cos(fx->pos.y_rot) * fx->speed) >> W2V_SHIFT;
    fx->counter++;
    if (fx->counter == 4) {
        fx->counter = 0;
        fx->frame_number--;
        if (fx->frame_number <= g_Objects[fx->object_number].nmeshes) {
            Effect_Kill(fx_num);
        }
    }
}
