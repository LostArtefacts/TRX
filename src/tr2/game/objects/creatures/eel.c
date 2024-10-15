#include "game/objects/creatures/eel.h"

#include "game/creature.h"
#include "game/objects/common.h"
#include "global/const.h"
#include "global/funcs.h"
#include "global/vars.h"

#define EEL_HITPOINTS 5

void Eel_Setup(void)
{
    OBJECT *const obj = &g_Objects[O_EEL];
    if (!obj->loaded) {
        return;
    }

    obj->control = Eel_Control;
    obj->collision = Creature_Collision;

    obj->hit_points = EEL_HITPOINTS;

    obj->water_creature = 1;
    obj->save_hitpoints = 1;
    obj->save_anim = 1;
}
