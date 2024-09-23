#include "game/objects/effects/twinkle.h"

#include "game/collide.h"
#include "game/effects.h"
#include "game/random.h"
#include "global/const.h"
#include "global/vars.h"

void Twinkle_Setup(OBJECT *obj)
{
    obj->control = Twinkle_Control;
}

void Twinkle_Control(int16_t fx_num)
{
    FX *fx = &g_Effects[fx_num];
    fx->counter++;
    if (fx->counter == 1) {
        fx->counter = 0;
        fx->frame_num--;
        if (fx->frame_num <= g_Objects[fx->object_id].nmeshes) {
            Effect_Kill(fx_num);
        }
    }
}

void Twinkle_Spawn(GAME_VECTOR *pos)
{
    int16_t fx_num = Effect_Create(pos->room_num);
    if (fx_num != NO_ITEM) {
        FX *fx = &g_Effects[fx_num];
        fx->pos.x = pos->x;
        fx->pos.y = pos->y;
        fx->pos.z = pos->z;
        fx->counter = 0;
        fx->object_id = O_TWINKLE;
        fx->frame_num = 0;
    }
}

void Twinkle_SparkleItem(ITEM *item, int mesh_mask)
{
    SPHERE slist[34];
    GAME_VECTOR effect_pos;

    int32_t num = Collide_GetSpheres(item, slist, 1);
    effect_pos.room_num = item->room_num;
    for (int i = 0; i < num; i++) {
        if (mesh_mask & (1 << i)) {
            SPHERE *sptr = &slist[i];
            effect_pos.x =
                sptr->x + sptr->r * (Random_GetDraw() - 0x4000) / 0x4000;
            effect_pos.y =
                sptr->y + sptr->r * (Random_GetDraw() - 0x4000) / 0x4000;
            effect_pos.z =
                sptr->z + sptr->r * (Random_GetDraw() - 0x4000) / 0x4000;
            Twinkle_Spawn(&effect_pos);
        }
    }
}
