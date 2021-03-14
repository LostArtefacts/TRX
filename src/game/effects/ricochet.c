#include "game/effects/ricochet.h"
#include "game/game.h"
#include "game/items.h"
#include "game/sound.h"
#include "game/vars.h"

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
        fx->frame_number = -3 * GetRandomDraw() / 0x8000;
        SoundEffect(SFX_LARA_RICOCHET, &fx->pos, SPM_NORMAL);
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
