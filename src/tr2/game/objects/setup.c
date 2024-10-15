#include "game/objects/setup.h"

#include "game/objects/common.h"
#include "global/funcs.h"
#include "global/types.h"

#define DEFAULT_RADIUS 10

void __cdecl Object_SetupAllObjects(void)
{
    for (int32_t i = 0; i < O_NUMBER_OF; i++) {
        OBJECT *const object = Object_GetObject(i);
        object->initialise = NULL;
        object->control = NULL;
        object->floor = NULL;
        object->ceiling = NULL;
        object->draw_routine = Object_DrawAnimatingItem;
        object->collision = NULL;
        object->hit_points = DONT_TARGET;
        object->pivot_length = 0;
        object->radius = DEFAULT_RADIUS;
        object->shadow_size = 0;

        object->save_position = 0;
        object->save_hitpoints = 0;
        object->save_flags = 0;
        object->save_anim = 0;
        object->intelligent = 0;
        object->water_creature = 0;
    }

    Object_SetupBaddyObjects();
    Object_SetupTrapObjects();
    Object_SetupGeneralObjects();

    InitialiseHair();
}
