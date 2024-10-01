#include "game/objects/effects/dart_effect.h"

#include "game/effects.h"
#include "game/objects/common.h"
#include "global/vars.h"

void DartEffect_Setup(OBJECT *obj)
{
    obj->control = DartEffect_Control;
    obj->draw_routine = Object_DrawSpriteItem;
}

void DartEffect_Control(int16_t fx_num)
{
    FX *fx = &g_Effects[fx_num];
    fx->counter++;
    if (fx->counter >= 3) {
        fx->counter = 0;
        fx->frame_num--;
        if (fx->frame_num <= g_Objects[fx->object_id].nmeshes) {
            Effect_Kill(fx_num);
        }
    }
}
