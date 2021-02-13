#include "game/data.h"
#include "game/draw.h"
#include "game/setup.h"
#include "game/types.h"
#include "util.h"

void __cdecl InitialiseObjects()
{
    for (int i = 0; i < NUMBER_OBJECTS; i++) {
        OBJECT_INFO* obj = &Objects[i];
        obj->intelligent = 0;
        obj->save_position = 0;
        obj->save_hitpoints = 0;
        obj->save_flags = 0;
        obj->save_anim = 0;
        obj->initialise = NULL;
        obj->collision = NULL;
        obj->control = NULL;
        obj->draw_routine = DrawAnimatingItem;
        obj->ceiling = NULL;
        obj->floor = NULL;
        obj->pivot_length = 0;
        obj->radius = DEFAULT_RADIUS;
        obj->shadow_size = 0;
        obj->hit_points = DONT_TARGET;
    }
    BaddyObjects();
    TrapObjects();
    ObjectObjects();
}

void TR1MInjectSetup()
{
    INJECT(0x00437A50, InitialiseObjects);
}
