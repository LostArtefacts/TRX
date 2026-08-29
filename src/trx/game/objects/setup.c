#include <trx/config.h>
#include <trx/game/lara.h>
#include <trx/game/objects/common.h>
#include <trx/game/objects/property.h>
#include <trx/game/pathing.h>

#define M_DEFAULT_RADIUS 10

static void M_SetupLara(OBJECT *const obj)
{
    obj->initialise_func = Lara_InitialiseLoad;
    obj->can_interpolate_func = Lara_CanInterpolate;
    obj->draw_func = nullptr;
    obj->get_mesh_index_func = Lara_GetMeshIndex;

    obj->shadow_size = (UNIT_SHADOW * 10) / 16;

    obj->save_position = true;
    obj->save_hitpoints = true;
    obj->save_flags = true;
    obj->save_anim = true;
    OBJECT_PROPERTIES(obj, ITEM_PROPERTY_MAX_HIT_POINTS(LARA_MAX_HITPOINTS));
    ObjectProperty_SetObjectValueRaw(
        obj, "max_hit_points",
        (TRX_VALUE) {
            .type = TVT_S32,
            .as_int = g_Config.gameplay.start_lara_hitpoints,
        });
}

static void M_SetupLaraStartPos(OBJECT *const obj)
{
    obj->draw_func = nullptr;
}

void Object_SetupAllObjects(void)
{
    CATALOG_FOR_EACH(CATALOG_OBJECTS, i)
    {
        OBJECT *const obj = Object_Get(i);
        obj->initialise_func = nullptr;
        obj->control_func = nullptr;
        obj->floor_height_func = nullptr;
        obj->ceiling_height_func = nullptr;
        obj->draw_func = Object_DrawAnimatingItem;
        obj->collision_func = nullptr;
        obj->add_walkable_func = nullptr;
        obj->is_usable_func = nullptr;
        obj->can_drop_items_func = nullptr;
        obj->can_interpolate_func = Object_CanInterpolate;
        obj->should_spawn_blood_func = nullptr;
        obj->is_alive_func = nullptr;
        obj->is_targetable_func = nullptr;
        obj->can_take_damage_func = nullptr;
        obj->can_be_projectile_target_func = nullptr;
        obj->can_be_exploded_func = nullptr;
        obj->get_mesh_index_func = nullptr;
        obj->pivot_length = 0;
        obj->radius = M_DEFAULT_RADIUS;
        obj->shadow_size = 0;
        obj->enable_interpolation = true;
        obj->lot_setup = LOT_Setup(LOT_SETUP_DEFAULT);

        obj->save_position = false;
        obj->save_hitpoints = false;
        obj->save_flags = false;
        obj->save_anim = false;
        obj->load_floor = false;
        obj->intelligent = false;
        obj->leaves_corpse = false;
        obj->smartness = -1;

        ObjectProperty_ResetObject(obj);
        if (obj->setup_func != nullptr) {
            obj->setup_func(obj);
        }
        obj->leaves_corpse |= obj->intelligent;

        // TODO: this is poor design
        OBJECT_PROPERTIES(
            obj,
            OBJECT_PROPERTY_STORED("ocb", 0, "Object configuration value."));
    }

    Lara_Hair_Initialise();
}

REGISTER_OBJECT(O_LARA, M_SetupLara)
REGISTER_OBJECT(O_LARA_START_POS, M_SetupLaraStartPos)
