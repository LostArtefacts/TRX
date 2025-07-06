#include "game/lara.h"
#include "game/objects/common.h"
#include "global/types.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/game/lara.h>

#define DEFAULT_RADIUS 10

static void M_SetupLara(void);

static void M_SetupLara(void)
{
    OBJECT *const obj = Object_Get(O_LARA);
    obj->initialise_func = Lara_InitialiseLoad;

    obj->shadow_size = (UNIT_SHADOW / 16) * 10;
    obj->hit_points = g_Config.gameplay.start_lara_hitpoints;
    obj->draw_func = Object_DrawDummyItem;

    obj->save_position = true;
    obj->save_hitpoints = true;
    obj->save_flags = true;
    obj->save_anim = true;
}

void Object_SetupAllObjects(void)
{
    for (int32_t i = O_FIRST; i < O_NUMBER_OF; i++) {
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
        obj->lot_setup = g_LOT_Default;

        obj->save_position = false;
        obj->save_hitpoints = false;
        obj->save_flags = false;
        obj->save_anim = false;
        obj->intelligent = false;

        if (obj->setup_func != nullptr) {
            obj->setup_func(obj);
        }
    }

    M_SetupLara();
    Lara_Hair_Initialise();
}
