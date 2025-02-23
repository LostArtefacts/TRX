#include "game/lara/common.h"
#include "game/lara/hair.h"
#include "game/objects/common.h"

#include <libtrx/config.h>

static void M_SetupLara(void);
static void M_SetupLaraExtra(void);
static void M_DisableObject(GAME_OBJECT_ID obj_id);

static void M_SetupLara(void)
{
    OBJECT *const obj = Object_Get(O_LARA);
    obj->initialise_func = Lara_InitialiseLoad;
    obj->draw_func = Object_DrawDummyItem;
    obj->hit_points = g_Config.gameplay.start_lara_hitpoints;
    obj->shadow_size = (UNIT_SHADOW * 10) / 16;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_anim = 1;
    obj->save_flags = 1;
}

static void M_SetupLaraExtra(void)
{
    OBJECT *const obj = Object_Get(O_LARA_EXTRA);
    obj->control_func = Lara_ControlExtra;
}

static void M_DisableObject(const GAME_OBJECT_ID obj_id)
{
    OBJECT *const obj = Object_Get(obj_id);
    obj->initialise_func = nullptr;
    obj->collision_func = nullptr;
    obj->control_func = nullptr;
    obj->draw_func = Object_DrawDummyItem;
    obj->floor_height_func = nullptr;
    obj->ceiling_height_func = nullptr;
}

void Object_SetupAllObjects(void)
{
    for (int i = 0; i < O_NUMBER_OF; i++) {
        OBJECT *const obj = Object_Get(i);
        obj->intelligent = 0;
        obj->save_position = 0;
        obj->save_hitpoints = 0;
        obj->save_flags = 0;
        obj->save_anim = 0;
        obj->initialise_func = nullptr;
        obj->collision_func = nullptr;
        obj->control_func = nullptr;
        obj->draw_func = Object_DrawAnimatingItem;
        obj->ceiling_height_func = nullptr;
        obj->floor_height_func = nullptr;
        obj->is_usable_func = nullptr;
        obj->pivot_length = 0;
        obj->radius = DEFAULT_RADIUS;
        obj->shadow_size = 0;
        obj->hit_points = DONT_TARGET;
        obj->enable_interpolation = true;

        if (obj->setup_func != nullptr) {
            obj->setup_func(obj);
        }
    }

    M_SetupLara();
    M_SetupLaraExtra();

    Lara_Hair_Initialise();

    if (g_Config.gameplay.disable_medpacks) {
        M_DisableObject(O_MEDI_ITEM);
        M_DisableObject(O_BIGMEDI_ITEM);
    }

    if (g_Config.gameplay.disable_magnums) {
        M_DisableObject(O_MAGNUM_ITEM);
        M_DisableObject(O_MAG_AMMO_ITEM);
    }

    if (g_Config.gameplay.disable_uzis) {
        M_DisableObject(O_UZI_ITEM);
        M_DisableObject(O_UZI_AMMO_ITEM);
    }

    if (g_Config.gameplay.disable_shotgun) {
        M_DisableObject(O_SHOTGUN_ITEM);
        M_DisableObject(O_SG_AMMO_ITEM);
    }
}
