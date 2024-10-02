#include "game/objects/effects/blood.h"

#include "game/effects.h"
#include "global/const.h"
#include "global/vars.h"
#include "math/math.h"

void Blood_Setup(OBJECT *obj)
{
    obj->control = Blood_Control;
}

void Blood_Control(int16_t fx_num)
{
    FX *fx = &g_Effects[fx_num];
    fx->pos.x += (Math_Sin(fx->rot.y) * fx->speed) >> W2V_SHIFT;
    fx->pos.z += (Math_Cos(fx->rot.y) * fx->speed) >> W2V_SHIFT;
    fx->counter++;
    if (fx->counter == 4) {
        fx->counter = 0;
        fx->frame_num--;
        if (fx->frame_num <= g_Objects[fx->object_id].nmeshes) {
            Effect_Kill(fx_num);
        }
    }
}
