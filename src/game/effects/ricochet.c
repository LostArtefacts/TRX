#include "game/effects/ricochet.h"

#include "game/items.h"
#include "game/random.h"
#include "game/sound.h"
#include "global/vars.h"

void SetupRicochet(OBJECT_INFO *obj)
{
    obj->control = ControlRicochet1;
}

void Ricochet(GAME_VECTOR *pos)
{
    int16_t fx_num = CreateEffect(pos->room_number);
    if (fx_num != NO_ITEM) {
        FX_INFO *fx = &Effects[fx_num];
        fx->pos.x = pos->x;
        fx->pos.y = pos->y;
        fx->pos.z = pos->z;
        fx->counter = 4;
        fx->object_number = O_RICOCHET1;
        fx->frame_number = -3 * Random_GetDraw() / 0x8000;
        Sound_Effect(SFX_LARA_RICOCHET, &fx->pos, SPM_NORMAL);
    }
}

void ControlRicochet1(int16_t fx_num)
{
    FX_INFO *fx = &Effects[fx_num];
    fx->counter--;
    if (!fx->counter) {
        KillEffect(fx_num);
    }
}
