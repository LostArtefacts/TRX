#include "game/lara.h"
#include "game/objects/common.h"

#include <libtrx/config.h>
#include <libtrx/game/lara/hair.h>

#define M_DEFAULT_RADIUS 10

static void M_SetupLara(void)
{
    OBJECT *const obj = Object_Get(O_LARA);
    obj->initialise_func = Lara_InitialiseLoad;
    obj->can_interpolate_func = Lara_CanInterpolate;
    obj->draw_func = nullptr;
    obj->hit_points = g_Config.gameplay.start_lara_hitpoints;
    obj->shadow_size = (UNIT_SHADOW * 10) / 16;
    obj->save_position = true;
    obj->save_hitpoints = true;
    obj->save_anim = true;
    obj->save_flags = true;
}

static void M_SetupSkybox(void)
{
    const OBJECT *const obj = Object_Get(O_SKYBOX);
    if (obj->loaded) {
        for (int32_t i = 0; i < obj->mesh_count; i++) {
            OBJECT_MESH *const obj_mesh = Object_GetMesh(obj->mesh_idx + i);
            obj_mesh->disable_transparency_sort = true;
        }
    }
}

static void M_DisableObject(const OBJECT_ID obj_id)
{
    OBJECT *const obj = Object_Get(obj_id);
    obj->initialise_func = nullptr;
    obj->collision_func = nullptr;
    obj->control_func = nullptr;
    obj->draw_func = nullptr;
    obj->floor_height_func = nullptr;
    obj->ceiling_height_func = nullptr;
    obj->add_walkable_func = nullptr;
}

void Object_SetupAllObjects(void)
{
    for (int i = O_FIRST; i < O_NUMBER_OF; i++) {
        OBJECT *const obj = Object_Get(i);
        obj->intelligent = false;
        obj->save_position = false;
        obj->save_hitpoints = false;
        obj->save_flags = false;
        obj->save_anim = false;
        obj->initialise_func = nullptr;
        obj->collision_func = nullptr;
        obj->control_func = nullptr;
        obj->draw_func = Object_DrawAnimatingItem;
        obj->ceiling_height_func = nullptr;
        obj->floor_height_func = nullptr;
        obj->is_usable_func = nullptr;
        obj->add_walkable_func = nullptr;
        obj->can_interpolate_func = Object_CanInterpolate;
        obj->pivot_length = 0;
        obj->radius = M_DEFAULT_RADIUS;
        obj->shadow_size = 0;
        obj->hit_points = DONT_TARGET;
        obj->enable_interpolation = true;
        obj->lot_setup = g_LOT_Default;

        if (obj->setup_func != nullptr) {
            obj->setup_func(obj);
        }
    }

    M_SetupLara();
    M_SetupSkybox();

    Lara_Hair_Initialise();

    if (g_Config.gameplay.disable_medpacks) {
        M_DisableObject(O_SMALL_MEDIPACK_ITEM);
        M_DisableObject(O_LARGE_MEDIPACK_ITEM);
    }

    if (g_Config.gameplay.disable_magnums) {
        M_DisableObject(O_MAGNUM_ITEM);
        M_DisableObject(O_MAGNUM_AMMO_ITEM);
    }

    if (g_Config.gameplay.disable_uzis) {
        M_DisableObject(O_UZI_ITEM);
        M_DisableObject(O_UZI_AMMO_ITEM);
    }

    if (g_Config.gameplay.disable_shotgun) {
        M_DisableObject(O_SHOTGUN_ITEM);
        M_DisableObject(O_SHOTGUN_AMMO_ITEM);
    }
}
