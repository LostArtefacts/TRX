#include "game/objects/creatures/trex.h"

#include "game/creature.h"
#include "game/objects/common.h"
#include "global/const.h"
#include "global/funcs.h"
#include "global/vars.h"

#define TREX_HITPOINTS 100
#define TREX_RADIUS (WALL_L / 3) // = 341

void TRex_Setup(void)
{
    OBJECT *const obj = &g_Objects[O_DINO];
    if (!obj->loaded) {
        return;
    }

    obj->control = TRex_Control;
    obj->collision = Creature_Collision;

    obj->hit_points = TREX_HITPOINTS;
    obj->radius = TREX_RADIUS;
    obj->shadow_size = 64;
    obj->pivot_length = 1800;

    obj->intelligent = 1;
    obj->save_anim = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_flags = 1;

    g_AnimBones[obj->bone_idx + 40] |= BF_ROT_Y;
}
