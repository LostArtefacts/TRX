#include "game/objects/effects/twinkle.h"

#include "game/collide.h"
#include "game/effects.h"
#include "game/random.h"
#include "game/spawn.h"
#include "global/const.h"
#include "global/vars.h"

static void M_Setup(OBJECT *obj);
static void M_Control(int16_t effect_num);

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
}

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
                sptr->x + sptr->r * (Random_GetDraw() - 0x4000) / 0x4000;
            effect_pos.y =
                sptr->y + sptr->r * (Random_GetDraw() - 0x4000) / 0x4000;
            effect_pos.z =
                sptr->z + sptr->r * (Random_GetDraw() - 0x4000) / 0x4000;
            Spawn_Twinkle(&effect_pos);
        }
    }
}

REGISTER_OBJECT(O_TWINKLE, M_Setup)
