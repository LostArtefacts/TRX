#include "game/objects/effects/gunshot.h"

#include "game/items.h"
#include "game/random.h"
#include "global/vars.h"

void GunShot_Setup(OBJECT_INFO *obj)
{
    obj->control = GunShot_Control;
}

void GunShot_Control(int16_t fx_num)
{
    FX_INFO *fx = &g_Effects[fx_num];
    fx->counter--;
    if (!fx->counter) {
        KillEffect(fx_num);
        return;
    }
    fx->pos.z_rot = Random_GetControl();
}
