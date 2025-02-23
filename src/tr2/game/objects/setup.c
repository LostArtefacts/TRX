#include "game/lara/control.h"
#include "game/lara/hair.h"
#include "game/objects/common.h"
#include "global/types.h"
#include "global/vars.h"

#define DEFAULT_RADIUS 10

static void M_SetupLara(void);
static void M_SetupLaraExtra(void);
static void M_SetupGeneralObjects(void);

static void M_SetupLara(void)
{
    OBJECT *const obj = Object_Get(O_LARA);
    obj->initialise_func = Lara_InitialiseLoad;

    obj->shadow_size = (UNIT_SHADOW / 16) * 10;
    obj->hit_points = LARA_MAX_HITPOINTS;
    obj->draw_func = Object_DrawDummyItem;

    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_flags = 1;
    obj->save_anim = 1;
}

static void M_SetupLaraExtra(void)
{
    OBJECT *const obj = Object_Get(O_LARA_EXTRA);
    obj->control_func = Lara_ControlExtra;
}

void Object_SetupAllObjects(void)
{
    for (int32_t i = 0; i < O_NUMBER_OF; i++) {
        OBJECT *const obj = Object_Get(i);
        obj->initialise_func = nullptr;
        obj->control_func = nullptr;
        obj->floor_height_func = nullptr;
        obj->ceiling_height_func = nullptr;
        obj->draw_func = Object_DrawAnimatingItem;
        obj->collision_func = nullptr;
        obj->hit_points = DONT_TARGET;
        obj->pivot_length = 0;
        obj->radius = DEFAULT_RADIUS;
        obj->shadow_size = 0;
        obj->enable_interpolation = true;

        obj->save_position = 0;
        obj->save_hitpoints = 0;
        obj->save_flags = 0;
        obj->save_anim = 0;
        obj->intelligent = 0;

        if (obj->setup_func != nullptr) {
            obj->setup_func(obj);
        }
    }

    M_SetupLara();
    M_SetupLaraExtra();
    Lara_Hair_Initialise();
}
