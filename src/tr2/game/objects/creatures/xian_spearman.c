#include "game/objects/creatures/xian_spearman.h"

#include "game/creature.h"
#include "game/objects/common.h"
#include "global/const.h"
#include "global/funcs.h"
#include "global/vars.h"

#include <assert.h>

#define XIAN_SPEARMAN_HITPOINTS 100
#define XIAN_SPEARMAN_RADIUS (WALL_L / 5) // = 204

void XianSpearman_Setup(void)
{
    OBJECT *const obj = &g_Objects[O_XIAN_SPEARMAN];
    if (!obj->loaded) {
        return;
    }

    assert(g_Objects[O_XIAN_SPEARMAN_STATUE].loaded);
    obj->initialise = XianKnight_Initialise;
    obj->draw_routine = XianKnight_Draw;
    obj->control = XianSpearman_Control;
    obj->collision = Creature_Collision;

    obj->hit_points = XIAN_SPEARMAN_HITPOINTS;
    obj->radius = XIAN_SPEARMAN_RADIUS;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->pivot_length = 0;

    obj->intelligent = 1;
    obj->save_anim = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_flags = 1;

    g_AnimBones[obj->bone_idx + 24] |= BF_ROT_Y;
}
