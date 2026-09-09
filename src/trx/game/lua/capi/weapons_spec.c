#include <trx/game/lua/capi/weapons_spec.h>

#include <trx/core/enum_map.h>
#include <trx/core/log.h>
#include <trx/core/math/const.h>
#include <trx/game/catalog/manager.h>
#include <trx/game/const.h>
#include <trx/game/gun/registry.h>
#include <trx/game/gun/routines.h>
#include <trx/game/objects/names.h>

#include <lauxlib.h>
#include <string.h>

#define M_MAX_NAME CATALOG_MAX_KEY_SIZE

// Names a routine of the given kind, and reports a name no module registers.
#define M_ROUTINE(L_, idx_, kind_, key_, target_)                              \
    do {                                                                       \
        char name_[M_MAX_NAME];                                                \
        bool found_ = false;                                                   \
        MUST(M_ReadString(                                                     \
            (L_), (idx_), (key_), name_, sizeof(name_), &found_));             \
        if (found_) {                                                          \
            const typeof(target_) func_ =                                      \
                CONCAT(Gun_Routines_Get, kind_)(name_);                        \
            FAIL_IF(                                                           \
                func_ == nullptr, "there is no %s routine '%s'", (key_),       \
                name_);                                                        \
            (target_) = func_;                                                 \
        }                                                                      \
    } while (0)

// Names one of an enumeration, and reports a name the enumeration lacks.
#define M_ENUM(L_, idx_, enum_name_, key_, target_)                            \
    do {                                                                       \
        char name_[M_MAX_NAME];                                                \
        bool found_ = false;                                                   \
        MUST(M_ReadString(                                                     \
            (L_), (idx_), (key_), name_, sizeof(name_), &found_));             \
        if (found_) {                                                          \
            const int32_t value_ = ENUM_MAP_GET(enum_name_, name_, -1);        \
            FAIL_IF(value_ < 0, "there is no %s '%s'", (key_), name_);         \
            (target_) = value_;                                                \
        }                                                                      \
    } while (0)

#define M_INT(L_, idx_, key_, target_)                                         \
    do {                                                                       \
        int64_t value_ = (int64_t)(target_);                                   \
        MUST(M_ReadInt((L_), (idx_), (key_), &value_));                        \
        (target_) = value_;                                                    \
    } while (0)

#define M_NUM(L_, idx_, key_, target_)                                         \
    do {                                                                       \
        double value_ = (double)(target_);                                     \
        MUST(M_ReadNumber((L_), (idx_), (key_), &value_));                     \
        (target_) = value_;                                                    \
    } while (0)

typedef RESULT (*M_GROUP_READER)(lua_State *L, int idx, WEAPON_INFO *w);

typedef struct {
    const char *key;
    M_GROUP_READER read;
} M_GROUP;

static bool M_Push(
    lua_State *const L, const int idx, const char *const key, int *const type)
{
    *type = lua_getfield(L, idx, key);
    if (*type == LUA_TNIL || *type == LUA_TNONE) {
        lua_pop(L, 1);
        return false;
    }
    return true;
}

static RESULT M_ReadInt(
    lua_State *const L, const int idx, const char *const key,
    int64_t *const target)
{
    int type = LUA_TNIL;
    if (!M_Push(L, idx, key, &type)) {
        return OK;
    }
    const bool is_int = lua_isinteger(L, -1);
    const int64_t held = lua_tointeger(L, -1);
    lua_pop(L, 1);
    FAIL_IF(!is_int, "'%s' must be a whole number", key);
    *target = held;
    return OK;
}

static RESULT M_ReadNumber(
    lua_State *const L, const int idx, const char *const key,
    double *const target)
{
    int type = LUA_TNIL;
    if (!M_Push(L, idx, key, &type)) {
        return OK;
    }
    const bool is_num = type == LUA_TNUMBER;
    const double held = lua_tonumber(L, -1);
    lua_pop(L, 1);
    FAIL_IF(!is_num, "'%s' must be a number", key);
    *target = held;
    return OK;
}

static RESULT M_ReadBool(
    lua_State *const L, const int idx, const char *const key,
    bool *const target)
{
    int type = LUA_TNIL;
    if (!M_Push(L, idx, key, &type)) {
        return OK;
    }
    const bool held = lua_toboolean(L, -1);
    lua_pop(L, 1);
    FAIL_IF(type != LUA_TBOOLEAN, "'%s' must be true or false", key);
    *target = held;
    return OK;
}

// Reads a name into `out`, treating an empty one as a name the spec does not
// state. The name is copied because the Lua string it comes from is reachable
// only while the value is on the stack.
static RESULT M_ReadString(
    lua_State *const L, const int idx, const char *const key, char *const out,
    const size_t size, bool *const found)
{
    *found = false;
    int type = LUA_TNIL;
    if (!M_Push(L, idx, key, &type)) {
        return OK;
    }
    RESULT result = OK;
    if (type != LUA_TSTRING) {
        result = FAIL("'%s' must be a name", key);
    } else {
        size_t len = 0;
        const char *const held = lua_tolstring(L, -1, &len);
        if (len >= size) {
            result = FAIL("'%s' must be shorter than %d", key, (int32_t)size);
        } else if (len > 0) {
            memcpy(out, held, len + 1);
            *found = true;
        }
    }
    lua_pop(L, 1);
    MUST(result);
    return OK;
}

static RESULT M_ReadList(
    lua_State *const L, const int idx, const char *const key,
    const char *const *const names, double *const out, const int32_t count,
    bool *const out_found)
{
    *out_found = false;
    int type = LUA_TNIL;
    if (!M_Push(L, idx, key, &type)) {
        return OK;
    }
    const int table = lua_gettop(L);
    RESULT result = OK;
    if (type != LUA_TTABLE) {
        result = FAIL("'%s' must hold %d numbers", key, count);
    } else if (lua_rawlen(L, table) > 0) {
        if ((int32_t)lua_rawlen(L, table) != count) {
            result = FAIL("'%s' must hold %d numbers", key, count);
        }
        for (int32_t i = 0; IS_OK(result) && i < count; i++) {
            if (lua_geti(L, table, i + 1) != LUA_TNUMBER) {
                result = FAIL("'%s' must hold %d numbers", key, count);
            } else {
                out[i] = lua_tonumber(L, -1);
            }
            lua_pop(L, 1);
        }
    } else {
        for (int32_t i = 0; IS_OK(result) && i < count; i++) {
            if (lua_getfield(L, table, names[i]) != LUA_TNUMBER) {
                result = FAIL("'%s' must name %s", key, names[i]);
            } else {
                out[i] = lua_tonumber(L, -1);
            }
            lua_pop(L, 1);
        }
    }
    lua_settop(L, table - 1);
    MUST(result);
    *out_found = true;
    return OK;
}

static RESULT M_ReadXYZ(
    lua_State *const L, const int idx, const char *const key,
    XYZ_32 *const target)
{
    static const char *const names[] = { "x", "y", "z" };
    double values[] = { target->x, target->y, target->z };
    bool found = false;
    MUST(M_ReadList(L, idx, key, names, values, ARRAY_SIZE(values), &found));
    if (found) {
        target->x = values[0];
        target->y = values[1];
        target->z = values[2];
    }
    return OK;
}

static RESULT M_ReadRGB(
    lua_State *const L, const int idx, const char *const key,
    RGB_F *const target)
{
    static const char *const names[] = { "r", "g", "b" };
    double values[] = { target->r, target->g, target->b };
    bool found = false;
    MUST(M_ReadList(L, idx, key, names, values, ARRAY_SIZE(values), &found));
    if (found) {
        target->r = values[0];
        target->g = values[1];
        target->b = values[2];
    }
    return OK;
}

static RESULT M_ReadAngles(
    lua_State *const L, const int idx, const char *const key,
    WEAPON_AIM_LIMITS *const limits)
{
    static const char *const names[] = { "min_yaw", "max_yaw", "min_pitch",
                                         "max_pitch" };
    double values[] = {
        limits->min_yaw / (double)DEG_1,
        limits->max_yaw / (double)DEG_1,
        limits->min_pitch / (double)DEG_1,
        limits->max_pitch / (double)DEG_1,
    };
    bool found = false;
    MUST(M_ReadList(L, idx, key, names, values, ARRAY_SIZE(values), &found));
    if (found) {
        limits->min_yaw = values[0] * DEG_1;
        limits->max_yaw = values[1] * DEG_1;
        limits->min_pitch = values[2] * DEG_1;
        limits->max_pitch = values[3] * DEG_1;
    }
    return OK;
}

static RESULT M_ReadObjectId(
    lua_State *const L, const int idx, const char *const key,
    OBJECT_ID *const target)
{
    char name[M_MAX_NAME];
    bool found = false;
    MUST(M_ReadString(L, idx, key, name, sizeof(name), &found));
    if (!found) {
        return OK;
    }
    const OBJECT_ID object_id = Object_IdFromKey(name);
    FAIL_IF(object_id == NO_OBJECT, "there is no object '%s'", name);
    *target = object_id;
    return OK;
}

static RESULT M_ReadSample(
    lua_State *const L, const int idx, const char *const key,
    SAMPLE_ID *const target)
{
    char name[M_MAX_NAME];
    bool found = false;
    MUST(M_ReadString(L, idx, key, name, sizeof(name), &found));
    if (!found) {
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

static RESULT M_ReadKeptString(
    lua_State *const L, const int idx, const char *const key,
    const char **const target)
{
    char name[M_MAX_NAME];
    bool found = false;
    MUST(M_ReadString(L, idx, key, name, sizeof(name), &found));
    if (found) {
        *target = Gun_Registry_KeepString(name);
    }
    return OK;
}

// Set the mesh source for Lara while she holds this weapon. An undeclared
// source uses the outfit's gun-swap object.
static RESULT M_ReadMeshes(
    lua_State *const L, const int idx, WEAPON_INFO *const w)
{
    OBJECT_ID object_id = NO_OBJECT;
    MUST(M_ReadObjectId(L, idx, "object", &object_id));
    if (object_id == NO_OBJECT) {
        return OK;
    }

    w->meshes = (WEAPON_MESH_INFO) {
        .is_declared = true,
        .object_id = object_id,
        .hand_r = -1,
        .hand_l = -1,
        .torso = -1,
        .thigh_r = -1,
        .thigh_l = -1,
    };
    M_INT(L, idx, "hand_r", w->meshes.hand_r);
    M_INT(L, idx, "hand_l", w->meshes.hand_l);
    M_INT(L, idx, "torso", w->meshes.torso);
    M_INT(L, idx, "thigh_r", w->meshes.thigh_r);
    M_INT(L, idx, "thigh_l", w->meshes.thigh_l);
    return OK;
}

static RESULT M_ReadObjects(
    lua_State *const L, const int idx, WEAPON_INFO *const w)
{
    MUST(M_ReadObjectId(L, idx, "pickup", &w->gun_object_id));
    MUST(M_ReadObjectId(L, idx, "ammo", &w->ammo_object_id));
    MUST(M_ReadObjectId(L, idx, "anim", &w->anim_object_id));
    MUST(M_ReadObjectId(L, idx, "shell", &w->shell_object_id));
    MUST(M_ReadObjectId(L, idx, "projectile", &w->projectile_object_id));
    return OK;
}

static RESULT M_ReadAmmo(
    lua_State *const L, const int idx, WEAPON_INFO *const w)
{
    WEAPON_AMMO_INFO *const ammo = &w->ammo;
    M_INT(L, idx, "initial_shots", ammo->initial_shots);
    M_INT(L, idx, "box_shots", ammo->box_shots);
    M_INT(L, idx, "box_label_qty", ammo->box_label_qty);
    M_INT(L, idx, "rounds_per_shot", ammo->rounds_per_shot);
    MUST(M_ReadBool(L, idx, "infinite", &ammo->infinite));
    MUST(M_ReadKeptString(L, idx, "icon", &w->ammo_icon));
    return OK;
}

static RESULT M_ReadAim(lua_State *const L, const int idx, WEAPON_INFO *const w)
{
    int64_t speed = w->aim_speed / DEG_1;
    int64_t accuracy = w->shot_accuracy / DEG_1;
    double target_dist = w->target_dist / (double)WALL_L;
    MUST(M_ReadInt(L, idx, "speed", &speed));
    MUST(M_ReadInt(L, idx, "accuracy", &accuracy));
    MUST(M_ReadNumber(L, idx, "target_dist", &target_dist));
    w->aim_speed = speed * DEG_1;
    w->shot_accuracy = accuracy * DEG_1;
    w->target_dist = target_dist * WALL_L;
    MUST(M_ReadAngles(L, idx, "lock", &w->lock));
    MUST(M_ReadAngles(L, idx, "left", &w->left_arm));
    MUST(M_ReadAngles(L, idx, "right", &w->right_arm));
    return OK;
}

static RESULT M_ReadAnim(
    lua_State *const L, const int idx, WEAPON_INFO *const w)
{
    WEAPON_ANIM_INFO *const anim = &w->anim;
    M_INT(L, idx, "equip", anim->equip_anim_idx);
    M_INT(L, idx, "draw_frame", anim->draw_frame);
    M_INT(L, idx, "undraw_frame", anim->undraw_frame);
    M_INT(L, idx, "recoil_frame", anim->recoil_frame);
    M_INT(L, idx, "shell_frame", anim->shell_frame);
    M_ROUTINE(L, idx, ReadyAnim, "ready", w->ready_anim_func);
    return OK;
}

static RESULT M_ReadFlash(
    lua_State *const L, const int idx, WEAPON_INFO *const w)
{
    WEAPON_FLASH_INFO *const flash = &w->flash;
    M_INT(L, idx, "time", flash->time);
    M_INT(L, idx, "shade", flash->shade);
    MUST(M_ReadRGB(L, idx, "color", &flash->color));
    MUST(M_ReadXYZ(L, idx, "pos", &flash->pos.right));
    MUST(M_ReadXYZ(L, idx, "pos_alt", &flash->pos.left));
    MUST(M_ReadBool(L, idx, "lights_room", &w->flash_lights_room));
    MUST(M_ReadBool(L, idx, "is_optional", &w->flash_is_optional));
    M_ROUTINE(L, idx, Flash, "routine", w->flash_func);
    return OK;
}

static RESULT M_ReadGlow(
    lua_State *const L, const int idx, WEAPON_INFO *const w)
{
    WEAPON_GLOW_INFO *const glow = &w->glow;
    MUST(M_ReadRGB(L, idx, "color", &glow->color));
    MUST(M_ReadXYZ(L, idx, "pos", &glow->pos));
    M_NUM(L, idx, "scale", glow->scale);
    MUST(M_ReadBool(L, idx, "flicker", &glow->flicker));
    return OK;
}

static RESULT M_ReadMuzzle(
    lua_State *const L, const int idx, WEAPON_INFO *const w)
{
    MUST(M_ReadXYZ(L, idx, "pos", &w->muzzle_pos.right));
    MUST(M_ReadXYZ(L, idx, "pos_alt", &w->muzzle_pos.left));
    return OK;
}

static RESULT M_ReadSmoke(
    lua_State *const L, const int idx, WEAPON_INFO *const w)
{
    MUST(M_ReadXYZ(L, idx, "pos", &w->smoke_pos.right));
    MUST(M_ReadXYZ(L, idx, "pos_alt", &w->smoke_pos.left));
    MUST(M_ReadXYZ(L, idx, "tip", &w->smoke_tip.right));
    MUST(M_ReadXYZ(L, idx, "tip_alt", &w->smoke_tip.left));
    M_INT(L, idx, "count", w->smoke_count);
    M_ROUTINE(L, idx, SmokeSize, "size", w->smoke_size_func);
    return OK;
}

static RESULT M_ReadShell(
    lua_State *const L, const int idx, WEAPON_INFO *const w)
{
    MUST(M_ReadXYZ(L, idx, "pos", &w->shell_pos.right));
    MUST(M_ReadXYZ(L, idx, "pos_alt", &w->shell_pos.left));
    MUST(M_ReadBool(L, idx, "throws_forward", &w->shell_throws_forward));
    M_INT(L, idx, "angle", w->shell_angle);
    M_INT(L, idx, "min_speed", w->shell_min_speed);
    return OK;
}

static RESULT M_ReadSound(
    lua_State *const L, const int idx, WEAPON_INFO *const w)
{
    MUST(M_ReadSample(L, idx, "fire", &w->sample_num));
    MUST(M_ReadSample(L, idx, "overlay", &w->sample_overlay_num));
    M_INT(L, idx, "overlay_pitch", w->sample_overlay_pitch);
    MUST(M_ReadBool(L, idx, "alternating", &w->has_alternating_fire_sound));
    M_ROUTINE(L, idx, Sound, "rapid_fire", w->rapid_fire_sound_func);
    return OK;
}

static RESULT M_ReadStow(
    lua_State *const L, const int idx, WEAPON_INFO *const w)
{
    M_INT(L, idx, "order", w->stow_order);
    M_ENUM(L, idx, STOW_PLACE, "place", w->stow_place);
    return OK;
}

static RESULT M_ReadSave(
    lua_State *const L, const int idx, WEAPON_INFO *const w)
{
    MUST(M_ReadKeptString(L, idx, "ammo_key", &w->save_ammo_key));
    MUST(M_ReadKeptString(L, idx, "resume_has_key", &w->save_resume_has_key));
    MUST(M_ReadKeptString(L, idx, "resume_ammo_key", &w->save_resume_ammo_key));
    MUST(M_ReadBool(L, idx, "required", &w->save_keys_required));
    return OK;
}

static RESULT M_ReadCheat(
    lua_State *const L, const int idx, WEAPON_INFO *const w)
{
    M_INT(L, idx, "ammo", w->cheat_ammo);
    M_INT(L, idx, "key_ammo", w->cheat_key_ammo);
    return OK;
}

// clang-format off
static const M_GROUP m_Groups[] = {
    { "objects", M_ReadObjects },
    { "meshes",  M_ReadMeshes },
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
    lua_State *const L, const int idx, const M_GROUP *const group,
    WEAPON_INFO *const w)
{
    int type = LUA_TNIL;
    if (!M_Push(L, idx, group->key, &type)) {
        return OK;
    }
    const int table = lua_gettop(L);
    RESULT result = OK;
    if (type != LUA_TTABLE) {
        result = FAIL("'%s' must hold a group of keys", group->key);
    } else {
        result = group->read(L, table, w);
    }
    lua_settop(L, table - 1);
    MUST(result, "'%s'", group->key);
    return OK;
}

// Reads the weapon this weapon is based on and keeps its own settings where
// they differ.
static RESULT M_ReadBase(
    lua_State *const L, const int idx, WEAPON_INFO *const w)
{
    char key[M_MAX_NAME];
    bool found = false;
    MUST(M_ReadString(L, idx, "base", key, sizeof(key), &found));
    if (!found) {
        return OK;
    }
    const CATALOG_ID base_id =
        Catalog_KeyToID(CATALOG_WEAPONS, key, NO_CATALOG_ID);
    FAIL_IF(
        base_id == NO_CATALOG_ID || !Gun_Registry_IsValidType(base_id),
        "there is no weapon '%s'", key);
    FAIL_IF(base_id == w->gun_type, "a weapon cannot be based on itself");

    const LARA_GUN_TYPE gun_type = w->gun_type;
    const bool was_declared = w->is_declared;
    const WEAPON_INFO own = *w;
    *w = *Gun_Registry_Get(base_id);
    w->gun_type = gun_type;
    w->skin_source = base_id;
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
static RESULT M_ReadKind(
    lua_State *const L, const int idx, WEAPON_INFO *const w)
{
    char name[M_MAX_NAME];
    bool found = false;
    MUST(M_ReadString(L, idx, "kind", name, sizeof(name), &found));
    if (!found) {
        return OK;
    }
    const int32_t kind = ENUM_MAP_GET(WEAPON_TYPE, name, -1);
    FAIL_IF(kind < 0, "there is no weapon kind '%s'", name);
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
    lua_State *const L, const int idx, WEAPON_INFO *const w,
    INPUT_ROLE *const out_role, int *const out_fire_idx)
{
    M_INT(L, idx, "damage", w->damage);
    M_INT(L, idx, "fire_delay", w->fire_delay);
    M_INT(L, idx, "gun_height", w->gun_height);
    MUST(M_ReadBool(L, idx, "is_available", &w->is_available));
    MUST(M_ReadBool(L, idx, "is_default", &w->is_default));
    MUST(M_ReadBool(L, idx, "is_remembered", &w->is_remembered));
    MUST(M_ReadBool(L, idx, "is_launcher", &w->is_launcher));
    MUST(M_ReadBool(L, idx, "is_machine_gun", &w->is_machine_gun));
    MUST(M_ReadBool(L, idx, "is_usable_underwater", &w->is_usable_underwater));
    MUST(M_ReadBool(L, idx, "wants_combat_camera", &w->wants_combat_camera));
    MUST(M_ReadBool(L, idx, "unaims_on_release", &w->unaims_on_release));
    // A script gives the fire routine of its own weapon as a function, which
    // stays on the stack until the whole spec reads, so that a spec that fails
    // leaves the weapon firing as it did.
    if (lua_getfield(L, idx, "fire") == LUA_TFUNCTION) {
        *out_fire_idx = lua_gettop(L);
    } else {
        lua_pop(L, 1);
        M_ROUTINE(L, idx, Fire, "fire", w->fire_func);
    }
    M_ENUM(L, idx, INPUT_ROLE, "equip_key", *out_role);
    return OK;
}

static RESULT M_Read(
    lua_State *const L, const int spec, WEAPON_INFO *const w,
    INPUT_ROLE *const out_role, int *const out_fire_idx)
{
    MUST(M_ReadBase(L, spec, w));
    MUST(M_ReadKind(L, spec, w));
    for (size_t i = 0; i < ARRAY_SIZE(m_Groups); i++) {
        MUST(M_ReadGroup(L, spec, &m_Groups[i], w));
    }
    MUST(M_ReadFlat(L, spec, w, out_role, out_fire_idx));
    return OK;
}

RESULT LUA_Weapons_ReadSpec(
    lua_State *const L, const int idx, WEAPON_INFO *const weapon)
{
    const int spec = lua_absindex(L, idx);
    FAIL_IF(!lua_istable(L, spec), "a weapon spec must be a table");

    const int top = lua_gettop(L);
    WEAPON_INFO copy = *weapon;
    INPUT_ROLE role = (INPUT_ROLE)-1;
    int fire_idx = 0;
    const RESULT result = M_Read(L, spec, &copy, &role, &fire_idx);
    if (!IS_OK(result)) {
        lua_settop(L, top);
        return result;
    }

    *weapon = copy;
    if ((int32_t)role >= 0) {
        Gun_Registry_SetInputRole(weapon->gun_type, role);
    }
    if (fire_idx != 0) {
        LUA_Weapons_SetFireHandler(L, weapon->gun_type, fire_idx);
    }
    lua_settop(L, top);
    return OK;
}
