#include <trx/game/gun/spec.h>

#include <trx/core/enum_map.h>
#include <trx/core/log.h>
#include <trx/core/math/const.h>
#include <trx/game/catalog/manager.h>
#include <trx/game/const.h>
#include <trx/game/gun/registry.h>
#include <trx/game/gun/routines.h>
#include <trx/game/objects/names.h>

// Names a routine of the given kind, and reports a name no module registers.
#define M_ROUTINE(io_, kind_, key_, target_)                                   \
    do {                                                                       \
        const char *name_ = nullptr;                                           \
        MUST(JSON_READ_OPT((io_), (key_), &name_));                            \
        if (name_ != nullptr) {                                                \
            const typeof(target_) func_ =                                      \
                CONCAT(Gun_Routines_Get, kind_)(name_);                        \
            if (func_ == nullptr) {                                            \
                return JSON_ReadIO_Fail(                                       \
                    (io_), "there is no %s routine '%s'", (key_), name_);      \
            }                                                                  \
            (target_) = func_;                                                 \
        }                                                                      \
    } while (0)

// Names one of an enumeration, and reports a name the enumeration lacks.
#define M_ENUM(io_, enum_name_, key_, target_)                                 \
    do {                                                                       \
        const char *name_ = nullptr;                                           \
        MUST(JSON_READ_OPT((io_), (key_), &name_));                            \
        if (name_ != nullptr) {                                                \
            const int32_t found_ = ENUM_MAP_GET(enum_name_, name_, -1);        \
            if (found_ < 0) {                                                  \
                return JSON_ReadIO_Fail(                                       \
                    (io_), "there is no %s '%s'", (key_), name_);              \
            }                                                                  \
            (target_) = found_;                                                \
        }                                                                      \
    } while (0)

typedef RESULT (*M_GROUP_READER)(JSON_READ_IO *io, WEAPON_INFO *w);

typedef struct {
    const char *key;
    M_GROUP_READER read;
} M_GROUP;

static RESULT M_ReadObjectId(
    JSON_READ_IO *const io, const char *const key, OBJECT_ID *const target)
{
    const char *name = nullptr;
    MUST(JSON_READ_OPT(io, key, &name));
    if (name == nullptr) {
        return OK;
    }
    const OBJECT_ID object_id = Object_IdFromKey(name);
    if (object_id == NO_OBJECT) {
        return JSON_ReadIO_Fail(io, "there is no object '%s'", name);
    }
    *target = object_id;
    return OK;
}

static RESULT M_ReadSample(
    JSON_READ_IO *const io, const char *const key, SAMPLE_ID *const target)
{
    const char *name = nullptr;
    MUST(JSON_READ_OPT(io, key, &name));
    if (name == nullptr) {
        return OK;
    }
    const CATALOG_ID sample_id =
        Catalog_KeyToID(CATALOG_SAMPLES, name, NO_CATALOG_ID);
    if (sample_id == NO_CATALOG_ID) {
        LOG_WARNING("there is no sample '%s'", name);
        return OK;
    }
    *target = sample_id;
    return OK;
}

static RESULT M_ReadString(
    JSON_READ_IO *const io, const char *const key, const char **const target)
{
    const char *name = nullptr;
    MUST(JSON_READ_OPT(io, key, &name));
    if (name != nullptr) {
        *target = Gun_Registry_KeepString(name);
    }
    return OK;
}

// Reads the four angles an arm or a lock is held to, in degrees.
static RESULT M_ReadAnglesCurrent(
    JSON_READ_IO *const io, WEAPON_AIM_LIMITS *const limits)
{
    int16_t *const angles[] = {
        &limits->min_yaw,
        &limits->max_yaw,
        &limits->min_pitch,
        &limits->max_pitch,
    };
    if (JSON_ARRAY_LEN(io) != (int32_t)ARRAY_SIZE(angles)) {
        return JSON_ReadIO_Fail(io, "must hold four angles");
    }
    for (size_t i = 0; i < ARRAY_SIZE(angles); i++) {
        int16_t degrees = 0;
        MUST(JSON_READ_A(io, i, &degrees));
        *angles[i] = degrees * DEG_1;
    }
    return OK;
}

static RESULT M_ReadAngles(
    JSON_READ_IO *const io, const char *const key,
    WEAPON_AIM_LIMITS *const limits)
{
    if (!JSON_ReadIO_HasKey(io, key)) {
        return OK;
    }
    MUST(JSON_PUSH(io, key));
    const RESULT result = M_ReadAnglesCurrent(io, limits);
    MUST(JSON_POP(io));
    return result;
}

// Reads a value the engine holds in game units, given in whole degrees.
static RESULT M_ReadDegrees(
    JSON_READ_IO *const io, const char *const key, int16_t *const target)
{
    int16_t degrees = *target / DEG_1;
    MUST(JSON_READ_OPT(io, key, &degrees));
    *target = degrees * DEG_1;
    return OK;
}

static RESULT M_ReadObjects(JSON_READ_IO *const io, WEAPON_INFO *const w)
{
    MUST(M_ReadObjectId(io, "pickup", &w->gun_object_id));
    MUST(M_ReadObjectId(io, "ammo", &w->ammo_object_id));
    MUST(M_ReadObjectId(io, "anim", &w->anim_object_id));
    MUST(M_ReadObjectId(io, "shell", &w->shell_object_id));
    MUST(M_ReadObjectId(io, "projectile", &w->projectile_object_id));
    return OK;
}

static RESULT M_ReadAmmo(JSON_READ_IO *const io, WEAPON_INFO *const w)
{
    WEAPON_AMMO_INFO *const ammo = &w->ammo;
    // TODO: remove the three older names after 1.14
    MUST(JSON_READ_OPT(io, "initial_qty", &ammo->initial_shots));
    MUST(JSON_READ_OPT(io, "pickup_qty", &ammo->box_shots));
    MUST(JSON_READ_OPT(io, "inventory_qty", &ammo->box_label_qty));
    MUST(JSON_READ_OPT(io, "initial_shots", &ammo->initial_shots));
    MUST(JSON_READ_OPT(io, "box_shots", &ammo->box_shots));
    MUST(JSON_READ_OPT(io, "box_label_qty", &ammo->box_label_qty));
    MUST(JSON_READ_OPT(io, "rounds_per_shot", &ammo->rounds_per_shot));
    MUST(JSON_READ_OPT(io, "infinite", &ammo->infinite));
    MUST(M_ReadString(io, "icon", &w->ammo_icon));
    return OK;
}

static RESULT M_ReadAim(JSON_READ_IO *const io, WEAPON_INFO *const w)
{
    MUST(M_ReadDegrees(io, "speed", &w->aim_speed));
    MUST(M_ReadDegrees(io, "accuracy", &w->shot_accuracy));
    double target_dist = w->target_dist / (double)WALL_L;
    MUST(JSON_READ_OPT(io, "target_dist", &target_dist));
    w->target_dist = target_dist * WALL_L;
    MUST(M_ReadAngles(io, "lock", &w->lock));
    MUST(M_ReadAngles(io, "left", &w->left_arm));
    MUST(M_ReadAngles(io, "right", &w->right_arm));
    return OK;
}

static RESULT M_ReadAnim(JSON_READ_IO *const io, WEAPON_INFO *const w)
{
    WEAPON_ANIM_INFO *const anim = &w->anim;
    MUST(JSON_READ_OPT(io, "equip", &anim->equip_anim_idx));
    MUST(JSON_READ_OPT(io, "draw_frame", &anim->draw_frame));
    MUST(JSON_READ_OPT(io, "undraw_frame", &anim->undraw_frame));
    MUST(JSON_READ_OPT(io, "recoil_frame", &anim->recoil_frame));
    MUST(JSON_READ_OPT(io, "shell_frame", &anim->shell_frame));
    M_ROUTINE(io, ReadyAnim, "ready", w->ready_anim_func);
    return OK;
}

static RESULT M_ReadFlash(JSON_READ_IO *const io, WEAPON_INFO *const w)
{
    WEAPON_FLASH_INFO *const flash = &w->flash;
    MUST(JSON_READ_OPT(io, "time", &flash->time));
    MUST(JSON_READ_OPT(io, "shade", &flash->shade));
    MUST(JSON_READ_OPT(io, "color", &flash->color));
    MUST(JSON_READ_OPT(io, "pos", &flash->pos.right));
    MUST(JSON_READ_OPT(io, "pos_alt", &flash->pos.left));
    MUST(JSON_READ_OPT(io, "lights_room", &w->flash_lights_room));
    MUST(JSON_READ_OPT(io, "is_optional", &w->flash_is_optional));
    M_ROUTINE(io, Flash, "routine", w->flash_func);
    return OK;
}

static RESULT M_ReadGlow(JSON_READ_IO *const io, WEAPON_INFO *const w)
{
    WEAPON_GLOW_INFO *const glow = &w->glow;
    MUST(JSON_READ_OPT(io, "color", &glow->color));
    MUST(JSON_READ_OPT(io, "pos", &glow->pos));
    MUST(JSON_READ_OPT(io, "scale", &glow->scale));
    MUST(JSON_READ_OPT(io, "flicker", &glow->flicker));
    return OK;
}

static RESULT M_ReadMuzzle(JSON_READ_IO *const io, WEAPON_INFO *const w)
{
    MUST(JSON_READ_OPT(io, "pos", &w->muzzle_pos.right));
    MUST(JSON_READ_OPT(io, "pos_alt", &w->muzzle_pos.left));
    return OK;
}

static RESULT M_ReadSmoke(JSON_READ_IO *const io, WEAPON_INFO *const w)
{
    MUST(JSON_READ_OPT(io, "pos", &w->smoke_pos.right));
    MUST(JSON_READ_OPT(io, "pos_alt", &w->smoke_pos.left));
    MUST(JSON_READ_OPT(io, "tip", &w->smoke_tip.right));
    MUST(JSON_READ_OPT(io, "tip_alt", &w->smoke_tip.left));
    MUST(JSON_READ_OPT(io, "count", &w->smoke_count));
    M_ROUTINE(io, SmokeSize, "size", w->smoke_size_func);
    return OK;
}

static RESULT M_ReadShell(JSON_READ_IO *const io, WEAPON_INFO *const w)
{
    MUST(JSON_READ_OPT(io, "pos", &w->shell_pos.right));
    MUST(JSON_READ_OPT(io, "pos_alt", &w->shell_pos.left));
    MUST(JSON_READ_OPT(io, "throws_forward", &w->shell_throws_forward));
    MUST(JSON_READ_OPT(io, "angle", &w->shell_angle));
    MUST(JSON_READ_OPT(io, "min_speed", &w->shell_min_speed));
    return OK;
}

static RESULT M_ReadSound(JSON_READ_IO *const io, WEAPON_INFO *const w)
{
    MUST(M_ReadSample(io, "fire", &w->sample_num));
    MUST(M_ReadSample(io, "overlay", &w->sample_overlay_num));
    MUST(JSON_READ_OPT(io, "overlay_pitch", &w->sample_overlay_pitch));
    MUST(JSON_READ_OPT(io, "alternating", &w->has_alternating_fire_sound));
    M_ROUTINE(io, Sound, "rapid_fire", w->rapid_fire_sound_func);
    return OK;
}

static RESULT M_ReadStow(JSON_READ_IO *const io, WEAPON_INFO *const w)
{
    MUST(JSON_READ_OPT(io, "order", &w->stow_order));
    M_ENUM(io, STOW_PLACE, "place", w->stow_place);
    return OK;
}

static RESULT M_ReadSave(JSON_READ_IO *const io, WEAPON_INFO *const w)
{
    MUST(M_ReadString(io, "ammo_key", &w->save_ammo_key));
    MUST(M_ReadString(io, "resume_has_key", &w->save_resume_has_key));
    MUST(M_ReadString(io, "resume_ammo_key", &w->save_resume_ammo_key));
    MUST(JSON_READ_OPT(io, "required", &w->save_keys_required));
    return OK;
}

static RESULT M_ReadCheat(JSON_READ_IO *const io, WEAPON_INFO *const w)
{
    MUST(JSON_READ_OPT(io, "ammo", &w->cheat_ammo));
    MUST(JSON_READ_OPT(io, "key_ammo", &w->cheat_key_ammo));
    return OK;
}

// clang-format off
static const M_GROUP m_Groups[] = {
    { "objects", M_ReadObjects },
    { "ammo",    M_ReadAmmo },
    { "aim",     M_ReadAim },
    { "anim",    M_ReadAnim },
    { "flash",   M_ReadFlash },
    { "glow",    M_ReadGlow },
    { "muzzle",  M_ReadMuzzle },
    { "smoke",   M_ReadSmoke },
    { "shell",   M_ReadShell },
    { "sound",   M_ReadSound },
    { "stow",    M_ReadStow },
    { "save",    M_ReadSave },
    { "cheat",   M_ReadCheat },
};
// clang-format on

static RESULT M_ReadGroup(
    JSON_READ_IO *const io, const M_GROUP *const group, WEAPON_INFO *const w)
{
    if (!JSON_ReadIO_HasKey(io, group->key)) {
        return OK;
    }
    MUST(JSON_PUSH(io, group->key));
    const RESULT result = group->read(io, w);
    MUST(JSON_POP(io));
    return result;
}

// Takes the weapon another weapon is built on as it stands, keeping what the
// engine gave this one where the two disagree.
static RESULT M_ReadBase(JSON_READ_IO *const io, WEAPON_INFO *const w)
{
    const char *key = nullptr;
    MUST(JSON_READ_OPT(io, "base", &key));
    if (key == nullptr) {
        return OK;
    }
    const CATALOG_ID base_id =
        Catalog_KeyToID(CATALOG_WEAPONS, key, NO_CATALOG_ID);
    if (base_id == NO_CATALOG_ID || !Gun_Registry_IsValidType(base_id)) {
        return JSON_ReadIO_Fail(io, "there is no weapon '%s'", key);
    }
    if (base_id == w->gun_type) {
        return JSON_ReadIO_Fail(io, "a weapon cannot be based on itself");
    }

    const LARA_GUN_TYPE gun_type = w->gun_type;
    const bool was_declared = w->is_declared;
    const WEAPON_INFO own = *w;
    *w = *Gun_Registry_Get(base_id);
    w->gun_type = gun_type;
    w->is_declared = was_declared;
    w->equip_input_role = was_declared ? own.equip_input_role : (INPUT_ROLE)-1;
    w->save_ammo_key = was_declared ? own.save_ammo_key : nullptr;
    w->save_resume_has_key = was_declared ? own.save_resume_has_key : nullptr;
    w->save_resume_ammo_key = was_declared ? own.save_resume_ammo_key : nullptr;
    w->save_keys_required = was_declared && own.save_keys_required;
    w->is_claimed = true;
    return OK;
}

// Names what drives the weapon, which declares it where the engine has an
// implementation for that kind and leaves it inert where it has none.
static RESULT M_ReadKind(JSON_READ_IO *const io, WEAPON_INFO *const w)
{
    const char *name = nullptr;
    MUST(JSON_READ_OPT(io, "kind", &name));
    if (name == nullptr) {
        return OK;
    }
    const int32_t kind = ENUM_MAP_GET(WEAPON_TYPE, name, -1);
    if (kind < 0) {
        return JSON_ReadIO_Fail(io, "there is no weapon kind '%s'", name);
    }
    if (w->is_declared) {
        return Gun_Registry_SetKind((WEAPON_TYPE)kind, w);
    }
    w->is_claimed = true;
    if (Gun_Routines_GetKind(kind) == nullptr) {
        w->type = (WEAPON_TYPE)kind;
        return OK;
    }
    return Gun_Registry_Declare(w->gun_type, (WEAPON_TYPE)kind, w);
}

static RESULT M_ReadFlat(
    JSON_READ_IO *const io, WEAPON_INFO *const w, INPUT_ROLE *const out_role)
{
    MUST(JSON_READ_OPT(io, "damage", &w->damage));
    MUST(JSON_READ_OPT(io, "gun_height", &w->gun_height));
    MUST(JSON_READ_OPT(io, "is_available", &w->is_available));
    MUST(JSON_READ_OPT(io, "is_default", &w->is_default));
    MUST(JSON_READ_OPT(io, "is_remembered", &w->is_remembered));
    MUST(JSON_READ_OPT(io, "is_launcher", &w->is_launcher));
    MUST(JSON_READ_OPT(io, "is_machine_gun", &w->is_machine_gun));
    MUST(JSON_READ_OPT(io, "is_usable_underwater", &w->is_usable_underwater));
    MUST(JSON_READ_OPT(io, "wants_combat_camera", &w->wants_combat_camera));
    MUST(JSON_READ_OPT(io, "unaims_on_release", &w->unaims_on_release));
    M_ROUTINE(io, Fire, "fire", w->fire_func);
    M_ENUM(io, INPUT_ROLE, "equip_key", *out_role);
    return OK;
}

RESULT Gun_Spec_Read(JSON_READ_IO *const io, WEAPON_INFO *const weapon)
{
    WEAPON_INFO copy = *weapon;
    INPUT_ROLE role = (INPUT_ROLE)-1;

    MUST(M_ReadBase(io, &copy));
    MUST(M_ReadKind(io, &copy));
    for (size_t i = 0; i < ARRAY_SIZE(m_Groups); i++) {
        MUST(M_ReadGroup(io, &m_Groups[i], &copy));
    }
    MUST(M_ReadFlat(io, &copy, &role));

    *weapon = copy;
    if ((int32_t)role >= 0) {
        Gun_Registry_SetInputRole(weapon->gun_type, role);
    }
    return OK;
}
