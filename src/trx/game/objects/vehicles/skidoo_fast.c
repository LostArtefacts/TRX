#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/game/music.h>
#include <trx/game/objects.h>
#include <trx/game/objects/vehicles/common.h>
#include <trx/game/objects/vehicles/skidoo_common.h>

static void M_PrivLoad(ITEM *const item, JSON_READ_IO *const io)
{
    SKIDOO_INFO *const p = item->priv;
    JSON_SHOULD(JSON_READ(io, "track_mesh", &p->track_mesh));
    JSON_SHOULD(JSON_READ(io, "skidoo_turn", &p->skidoo_turn));
    JSON_SHOULD(JSON_READ(io, "left_fallspeed", &p->left_fallspeed));
    JSON_SHOULD(JSON_READ(io, "right_fallspeed", &p->right_fallspeed));
    JSON_SHOULD(JSON_READ(io, "momentum_angle", &p->momentum_angle));
    JSON_SHOULD(JSON_READ(io, "extra_rotation", &p->extra_rotation));
    JSON_SHOULD(JSON_READ(io, "pitch", &p->pitch));
}

static void M_PrivSave(const ITEM *const item, JSON_WRITE_IO *const io)
{
    const SKIDOO_INFO *const p = item->priv;
    JSONW_WRITE(io, "track_mesh", p->track_mesh);
    JSONW_WRITE(io, "skidoo_turn", p->skidoo_turn);
    JSONW_WRITE(io, "left_fallspeed", p->left_fallspeed);
    JSONW_WRITE(io, "right_fallspeed", p->right_fallspeed);
    JSONW_WRITE(io, "momentum_angle", p->momentum_angle);
    JSONW_WRITE(io, "extra_rotation", p->extra_rotation);
    JSONW_WRITE(io, "pitch", p->pitch);
}

static void M_Setup(OBJECT *const obj)
{
    obj->initialise_func = Skidoo_Initialise;
    obj->draw_func = Skidoo_Draw;
    obj->collision_func = Skidoo_Collision;
    obj->event_func = Vehicle_HandleEvent;
    obj->priv_size = sizeof(SKIDOO_INFO);
    obj->priv_load_func = M_PrivLoad;
    obj->priv_save_func = M_PrivSave;
    obj->save_position = true;
    obj->save_flags = true;
    obj->save_anim = true;

    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY_INT(
            "track_1", Music_ToGameID(MX_SKIDOO_THEME),
            "Random music track pool, slot 1. -1 = disabled."),
        OBJECT_PROPERTY_INT(
            "track_2", -1, "Random music track pool, slot 2. -1 = disabled."),
        OBJECT_PROPERTY_INT(
            "track_3", -1, "Random music track pool, slot 3. -1 = disabled."),
        OBJECT_PROPERTY_INT(
            "track_4", -1, "Random music track pool, slot 4. -1 = disabled."),
        OBJECT_PROPERTY_INT(
            "battle_track_1", Music_ToGameID(MX_BATTLE_THEME),
            "Random battle music track pool, slot 1. -1 = disabled."),
        OBJECT_PROPERTY_INT(
            "battle_track_2", -1,
            "Random battle music track pool, slot 2. -1 = disabled."),
        OBJECT_PROPERTY_INT(
            "battle_track_3", -1,
            "Random battle music track pool, slot 3. -1 = disabled."),
        OBJECT_PROPERTY_INT(
            "battle_track_4", -1,
            "Random battle music track pool, slot 4. -1 = disabled."),
        OBJECT_PROPERTY_BOOL(
            "is_heavy", true,
            "Whether or not this vehicle can activate heavy triggers."),
        OBJECT_PROPERTY_BOOL(
            "test_static_collision", false,
            "Whether or not this vehicle can collide with static meshes."));
}

REGISTER_OBJECT(O_SKIDOO_FAST, M_Setup)
