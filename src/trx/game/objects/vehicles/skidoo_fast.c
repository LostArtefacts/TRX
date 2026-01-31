#include <trx/game/objects.h>
#include <trx/game/objects/vehicles/skidoo_common.h>
#include <trx/game/savegame/bson_read_io.h>
#include <trx/game/savegame/bson_write_io.h>
#include <trx/game/savegame/legacy_io.h>

static void M_PrivLoad(ITEM *const item, SG_READ_IO *const io)
{
    SKIDOO_INFO *const p = item->priv;
    SG_SHOULD(SG_READ_VALUE(io, "track_mesh", &p->track_mesh));
    SG_SHOULD(SG_READ_VALUE(io, "skidoo_turn", &p->skidoo_turn));
    SG_SHOULD(SG_READ_VALUE(io, "left_fallspeed", &p->left_fallspeed));
    SG_SHOULD(SG_READ_VALUE(io, "right_fallspeed", &p->right_fallspeed));
    SG_SHOULD(SG_READ_VALUE(io, "momentum_angle", &p->momentum_angle));
    SG_SHOULD(SG_READ_VALUE(io, "extra_rotation", &p->extra_rotation));
    SG_SHOULD(SG_READ_VALUE(io, "pitch", &p->pitch));
}

static void M_PrivSave(const ITEM *const item, SG_WRITE_IO *const io)
{
    const SKIDOO_INFO *const p = item->priv;
    SGW_WRITE_VALUE(io, "track_mesh", p->track_mesh);
    SGW_WRITE_VALUE(io, "skidoo_turn", p->skidoo_turn);
    SGW_WRITE_VALUE(io, "left_fallspeed", p->left_fallspeed);
    SGW_WRITE_VALUE(io, "right_fallspeed", p->right_fallspeed);
    SGW_WRITE_VALUE(io, "momentum_angle", p->momentum_angle);
    SGW_WRITE_VALUE(io, "extra_rotation", p->extra_rotation);
    SGW_WRITE_VALUE(io, "pitch", p->pitch);
}

static void M_PrivLegacyLoad(
    ITEM *const item, const SAVEGAME_LEGACY_IO *const io)
{
    SKIDOO_INFO *const p = item->priv;
    p->track_mesh = io->read_s16(io);
    p->skidoo_turn = io->read_s32(io);
    p->left_fallspeed = io->read_s32(io);
    p->right_fallspeed = io->read_s32(io);
    p->momentum_angle = io->read_s16(io);
    p->extra_rotation = io->read_s16(io);
    p->pitch = io->read_s32(io);
}

static void M_Setup(OBJECT *const obj)
{
    obj->initialise_func = Skidoo_Initialise;
    obj->draw_func = Skidoo_Draw;
    obj->collision_func = Skidoo_Collision;
    obj->priv_size = sizeof(SKIDOO_INFO);
    obj->priv_load_func = M_PrivLoad;
    obj->priv_save_func = M_PrivSave;
    obj->priv_legacy_load_func = M_PrivLegacyLoad;
    obj->save_position = true;
    obj->save_flags = true;
    obj->save_anim = true;
}

REGISTER_OBJECT(O_SKIDOO_FAST, M_Setup)
