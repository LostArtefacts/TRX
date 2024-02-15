#include "game/objects/effects/gunshot.h"

#include "game/effects.h"
#include "game/random.h"

void GunShot_Setup(OBJECT_INFO *obj)
{
    obj->control = GunShot_Control;
}

void GunShot_Control(int16_t fx_num)
{
    FX_INFO *fx = &g_Effects[fx_num];
    fx->counter--;
    if (!fx->counter) {
        Effect_Kill(fx_num);
        return;
    }
    fx->rot.z = Random_GetControl();
}
