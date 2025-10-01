#include "game/objects/effects/twinkle.h"

#include "game/effects.h"
#include "game/spawn.h"

#include <libtrx/game/collision.h>
#include <libtrx/game/random.h>

static void M_Control(const int16_t effect_num)
{
    EFFECT *effect = Effect_Get(effect_num);
    effect->counter++;
    if (effect->counter == 1) {
        effect->counter = 0;
        effect->frame_num--;
        if (effect->frame_num <= Object_Get(effect->object_id)->mesh_count) {
            Effect_Kill(effect_num);
        }
    }
}

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
}

void Twinkle_SparkleItem(ITEM *const item, uint32_t mesh_mask)
{
    SPHERE slist[34];
    GAME_VECTOR effect_pos;

    int32_t num = Collide_GetSpheres(item, slist, 1);
    effect_pos.room_num = item->room_num;
    for (int i = 0; i < num; i++) {
        if (mesh_mask & (1 << i)) {
            SPHERE *sptr = &slist[i];
            effect_pos.x =
                sptr->pos.x + sptr->r * (Random_GetDraw() - 0x4000) / 0x4000;
            effect_pos.y =
                sptr->pos.y + sptr->r * (Random_GetDraw() - 0x4000) / 0x4000;
            effect_pos.z =
                sptr->pos.z + sptr->r * (Random_GetDraw() - 0x4000) / 0x4000;
            Spawn_Twinkle(&effect_pos);
        }
    }
}

REGISTER_OBJECT(O_TWINKLE, M_Setup)
