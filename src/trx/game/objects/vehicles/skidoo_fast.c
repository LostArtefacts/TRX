#include <trx/game/objects.h>
#include <trx/game/objects/vehicles/skidoo_common.h>
#include <trx/game/savegame/legacy_io.h>

static void M_PrivLoad(ITEM *const item, const JSON_OBJECT *const root)
{
    SKIDOO_INFO *const p = item->priv;
    p->track_mesh = JSON_ObjectGetInt(root, "track_mesh", p->track_mesh);
    p->skidoo_turn = JSON_ObjectGetInt(root, "skidoo_turn", p->skidoo_turn);
    p->left_fallspeed =
        JSON_ObjectGetInt(root, "left_fallspeed", p->left_fallspeed);
    p->right_fallspeed =
        JSON_ObjectGetInt(root, "right_fallspeed", p->right_fallspeed);
    p->momentum_angle =
        JSON_ObjectGetInt(root, "momentum_angle", p->momentum_angle);
    p->extra_rotation =
        JSON_ObjectGetInt(root, "extra_rotation", p->extra_rotation);
    p->pitch = JSON_ObjectGetInt(root, "pitch", p->pitch);
}

static void M_PrivSave(const ITEM *const item, JSON_OBJECT *const root)
{
    const SKIDOO_INFO *const p = item->priv;
    JSON_ObjectAppendInt(root, "track_mesh", p->track_mesh);
    JSON_ObjectAppendInt(root, "skidoo_turn", p->skidoo_turn);
    JSON_ObjectAppendInt(root, "left_fallspeed", p->left_fallspeed);
    JSON_ObjectAppendInt(root, "right_fallspeed", p->right_fallspeed);
    JSON_ObjectAppendInt(root, "momentum_angle", p->momentum_angle);
    JSON_ObjectAppendInt(root, "extra_rotation", p->extra_rotation);
    JSON_ObjectAppendInt(root, "pitch", p->pitch);
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
