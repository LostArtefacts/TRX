#include "game/objects/creatures/yeti.h"

#include "game/creature.h"
#include "game/objects/common.h"
#include "global/const.h"
#include "global/funcs.h"
#include "global/vars.h"

#define YETI_HITPOINTS 30
#define YETI_RADIUS (WALL_L / 8) // = 128

void Yeti_Setup(void)
{
    OBJECT *const obj = &g_Objects[O_YETI];
    if (!obj->loaded) {
        return;
    }

    obj->control = Yeti_Control;
    obj->collision = Creature_Collision;

    obj->hit_points = YETI_HITPOINTS;
    obj->radius = YETI_RADIUS;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->pivot_length = 100;

    obj->intelligent = 1;
    obj->save_anim = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_flags = 1;

    g_AnimBones[obj->bone_idx + 24] |= BF_ROT_Y;
}
