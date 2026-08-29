#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/game/music.h>
#include <trx/game/objects.h>
#include <trx/game/objects/vehicles/common.h>
#include <trx/game/objects/vehicles/skidoo_common.h>

static RESULT M_PrivLoad(ITEM *const item, JSON_READ_IO *const io)
{
    SKIDOO_INFO *const p = item->priv;
    MUST(JSON_READ_OPT(io, "track_mesh", &p->track_mesh));
    MUST(JSON_READ_OPT(io, "skidoo_turn", &p->skidoo_turn));
    MUST(JSON_READ_OPT(io, "left_fallspeed", &p->left_fallspeed));
    MUST(JSON_READ_OPT(io, "right_fallspeed", &p->right_fallspeed));
    MUST(JSON_READ_OPT(io, "momentum_angle", &p->momentum_angle));
    MUST(JSON_READ_OPT(io, "extra_rotation", &p->extra_rotation));
    MUST(JSON_READ_OPT(io, "pitch", &p->pitch));
    return OK;
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
        OBJECT_PROPERTY_STORED(
            "track_1", Music_IDToSlot(MX_SKIDOO_THEME),
            "Random music track pool, slot 1. -1 = disabled."),
        OBJECT_PROPERTY_STORED(
            "track_2", -1, "Random music track pool, slot 2. -1 = disabled."),
        OBJECT_PROPERTY_STORED(
            "track_3", -1, "Random music track pool, slot 3. -1 = disabled."),
        OBJECT_PROPERTY_STORED(
            "track_4", -1, "Random music track pool, slot 4. -1 = disabled."),
        OBJECT_PROPERTY_STORED(
            "battle_track_1", Music_IDToSlot(MX_BATTLE_THEME),
            "Random battle music track pool, slot 1. -1 = disabled."),
        OBJECT_PROPERTY_STORED(
            "battle_track_2", -1,
            "Random battle music track pool, slot 2. -1 = disabled."),
        OBJECT_PROPERTY_STORED(
            "battle_track_3", -1,
            "Random battle music track pool, slot 3. -1 = disabled."),
        OBJECT_PROPERTY_STORED(
            "battle_track_4", -1,
            "Random battle music track pool, slot 4. -1 = disabled."),
        OBJECT_PROPERTY_STORED(
            "is_heavy", true,
            "Whether or not this vehicle can activate heavy triggers."),
        OBJECT_PROPERTY(
            SKIDOO_INFO, test_static_collision, false,
            "Whether or not this vehicle can collide with static meshes."));
}

REGISTER_OBJECT(O_SKIDOO_FAST, M_Setup)
