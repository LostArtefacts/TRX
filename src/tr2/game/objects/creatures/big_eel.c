#include "game/objects/creatures/big_eel.h"

#include "game/creature.h"
#include "game/objects/common.h"
#include "global/const.h"
#include "global/funcs.h"

#define BIG_EEL_HITPOINTS 20

void BigEel_Setup(void)
{
    OBJECT *const obj = Object_GetObject(O_BIG_EEL);
    if (!obj->loaded) {
        return;
    }

    obj->control = BigEel_Control;
    obj->collision = Creature_Collision;

    obj->hit_points = BIG_EEL_HITPOINTS;

    obj->water_creature = 1;
    obj->save_hitpoints = 1;
    obj->save_anim = 1;
    obj->save_flags = 1;
}
