#include "game/effects/twinkle.h"

#include "game/items.h"
#include "game/random.h"
#include "game/sphere.h"
#include "global/vars.h"

void SetupTwinkle(OBJECT_INFO *obj)
{
    obj->control = ControlTwinkle;
}

void Twinkle(GAME_VECTOR *pos)
{
    int16_t fx_num = CreateEffect(pos->room_number);
    if (fx_num != NO_ITEM) {
        FX_INFO *fx = &Effects[fx_num];
        fx->pos.x = pos->x;
        fx->pos.y = pos->y;
        fx->pos.z = pos->z;
        fx->counter = 0;
        fx->object_number = O_TWINKLE;
        fx->frame_number = 0;
    }
}

void ItemSparkle(ITEM_INFO *item, int meshmask)
{
    SPHERE slist[34];
    GAME_VECTOR effect_pos;

    int32_t num = GetSpheres(item, slist, 1);
    effect_pos.room_number = item->room_number;
    for (int i = 0; i < num; i++) {
        if (meshmask & (1 << i)) {
            SPHERE *sptr = &slist[i];
            effect_pos.x =
                sptr->x + sptr->r * (Random_GetDraw() - 0x4000) / 0x4000;
            effect_pos.y =
                sptr->y + sptr->r * (Random_GetDraw() - 0x4000) / 0x4000;
            effect_pos.z =
                sptr->z + sptr->r * (Random_GetDraw() - 0x4000) / 0x4000;
            Twinkle(&effect_pos);
        }
    }
}

void ControlTwinkle(int16_t fx_num)
{
    FX_INFO *fx = &Effects[fx_num];
    fx->counter++;
    if (fx->counter == 1) {
        fx->counter = 0;
        fx->frame_number--;
        if (fx->frame_number <= Objects[fx->object_number].nmeshes) {
            KillEffect(fx_num);
        }
    }
}
