#include "game/objects/effects/ricochet.h"

#include "game/effects.h"
#include "game/random.h"
#include "game/sound.h"
#include "global/const.h"

void Ricochet_Setup(OBJECT *obj)
{
    obj->control = Ricochet_Control;
}

void Ricochet_Control(int16_t fx_num)
{
    FX *fx = &g_Effects[fx_num];
    fx->counter--;
    if (!fx->counter) {
        Effect_Kill(fx_num);
    }
}

void Ricochet_Spawn(GAME_VECTOR *pos)
{
    int16_t fx_num = Effect_Create(pos->room_num);
    if (fx_num != NO_ITEM) {
        FX *fx = &g_Effects[fx_num];
        fx->pos.x = pos->x;
        fx->pos.y = pos->y;
        fx->pos.z = pos->z;
        fx->counter = 4;
        fx->object_id = O_RICOCHET_1;
        fx->frame_num = -3 * Random_GetDraw() / 0x8000;
        Sound_Effect(SFX_LARA_RICOCHET, &fx->pos, SPM_NORMAL);
    }
}
