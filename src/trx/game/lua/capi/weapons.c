#include <trx/game/gun.h>
#include <trx/game/gun/common.h>
#include <trx/game/gun/registry.h>
#include <trx/game/lua/common.h>
#include <trx/game/lua/field.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/struct.h>
#include <trx/game/lua/utils.h>
#include <trx/game/objects.h>

#include <lauxlib.h>

// What a weapon is, rather than what anyone has of it. None of this differs
// between the inventory Lara carries and the one a level keeps for her, which
// is why it belongs to neither.
//
// A weapon is addressed by its LARA_GUN_TYPE, which is valid for the whole
// session: the table is read once, when the mod's weapons.json5 is, so a handle
// never goes stale and what a script writes lasts until the game restarts.
//
// The groups a weapon is made of - its aim limits, its ammunition, its flash -
// are structs, and a struct is not a value a field can hold. Each is registered
// as a type of its own and reached through a handle that names the weapon it
// belongs to, with the generation saying which of the group it is where a
// weapon has several.

typedef enum {
    M_LIMITS_LOCK,
    M_LIMITS_LEFT_ARM,
    M_LIMITS_RIGHT_ARM,
} M_LIMITS_KIND;

typedef enum {
    M_HAND_POS_MUZZLE,
    M_HAND_POS_SHELL,
    M_HAND_POS_FLASH,
} M_HAND_POS_KIND;

static const WEAPON_INFO *M_WeaponOfAnim(const void *anim);
static bool M_GetID(const void *self, TRX_VALUE *out);
static const char *M_SetKind(void *self, const TRX_VALUE *in);
static const char *M_SetSample(void *self, const TRX_VALUE *in);
static const char *M_SetOverlaySample(void *self, const TRX_VALUE *in);
static const char *M_SetEquipAnim(void *self, const TRX_VALUE *in);

// clang-format off
static const FIELD_DESC m_LimitsFields[] = {
    FIELD_MODULAR(WEAPON_AIM_LIMITS, min_yaw),
    FIELD_MODULAR(WEAPON_AIM_LIMITS, max_yaw),
    FIELD_MODULAR(WEAPON_AIM_LIMITS, min_pitch),
    FIELD_MODULAR(WEAPON_AIM_LIMITS, max_pitch),
};

static const FIELD_DESC m_HandPosFields[] = {
    FIELD(WEAPON_HAND_POS, right),
    FIELD(WEAPON_HAND_POS, left),
};

static const FIELD_DESC m_AmmoFields[] = {
    FIELD(WEAPON_AMMO_INFO, initial_shots),
    FIELD(WEAPON_AMMO_INFO, box_shots),
    FIELD(WEAPON_AMMO_INFO, box_label_qty),
    FIELD(WEAPON_AMMO_INFO, infinite),
};

static const FIELD_DESC m_FlashFields[] = {
    FIELD(WEAPON_FLASH_INFO, time),
    FIELD(WEAPON_FLASH_INFO, shade),
    FIELD(WEAPON_FLASH_INFO, color),
};

static const FIELD_DESC m_GlowFields[] = {
    FIELD(WEAPON_GLOW_INFO, color),
    FIELD(WEAPON_GLOW_INFO, pos),
    FIELD(WEAPON_GLOW_INFO, scale),
    FIELD(WEAPON_GLOW_INFO, flicker),
};

static const FIELD_DESC m_AnimFields[] = {
    FIELD_SET(WEAPON_ANIM_INFO, equip_anim_idx, M_SetEquipAnim),
    FIELD(WEAPON_ANIM_INFO, draw_frame),
    FIELD(WEAPON_ANIM_INFO, undraw_frame),
    FIELD(WEAPON_ANIM_INFO, recoil_frame),
};

static const FIELD_DESC m_WeaponFields[] = {
    FIELD_FN("id", TVT_S32, M_GetID, nullptr),
    FIELD_SET(WEAPON_INFO, type, M_SetKind),
    FIELD(WEAPON_INFO, is_available),

    // how it aims and what it does when it hits
    FIELD(WEAPON_INFO, aim_speed),
    FIELD(WEAPON_INFO, shot_accuracy),
    FIELD(WEAPON_INFO, gun_height),
    FIELD(WEAPON_INFO, damage),
    FIELD(WEAPON_INFO, target_dist),
    FIELD(WEAPON_INFO, smoke_count),
    FIELD_SET(WEAPON_INFO, sample_num, M_SetSample),
    FIELD_SET(WEAPON_INFO, sample_overlay_num, M_SetOverlaySample),
    FIELD(WEAPON_INFO, sample_overlay_pitch),

    // Deliberately absent: the groups. lock, left_arm, right_arm, ammo, anim,
    // flash, glow, muzzle_pos and shell_pos are structs, and each is reached as
    // a handle of its own type.
};
// clang-format on

TYPE_DEFINE(WEAPON_AIM_LIMITS, m_LimitsFields)
TYPE_DEFINE(WEAPON_HAND_POS, m_HandPosFields)
TYPE_DEFINE(WEAPON_AMMO_INFO, m_AmmoFields)
TYPE_DEFINE(WEAPON_FLASH_INFO, m_FlashFields)
TYPE_DEFINE(WEAPON_GLOW_INFO, m_GlowFields)
TYPE_DEFINE(WEAPON_ANIM_INFO, m_AnimFields)
TYPE_DEFINE(WEAPON_INFO, m_WeaponFields)

static LARA_GUN_TYPE M_GetWeapon(lua_State *const L, const int arg)
{
    const lua_Integer gun_type = luaL_checkinteger(L, arg);
    if (gun_type <= LGT_UNARMED || !Gun_Registry_IsValidType(gun_type)) {
        luaL_argerror(L, arg, "not a weapon");
    }
    return (LARA_GUN_TYPE)gun_type;
}

// The weapon an animation group belongs to. The group has no way back to it of
// its own, and validating an animation number needs the weapon's own object.
static const WEAPON_INFO *M_WeaponOfAnim(const void *const anim)
{
    return (const WEAPON_INFO *)((const char *)anim
                                 - offsetof(WEAPON_INFO, anim));
}

// Which weapon this is, which is what lets a script go from a weapon back to
// everything that takes one.
static bool M_GetID(const void *const self, TRX_VALUE *const out)
{
    *out = (TRX_VALUE) {
        .type = TVT_S32,
        .as_int = ((const WEAPON_INFO *)self)->gun_type,
    };
    return true;
}

static const char *M_SetKind(void *const self, const TRX_VALUE *const in)
{
    if (in->as_int < 0 || in->as_int >= NUM_WEAPON_TYPES) {
        return "unknown weapon kind";
    }
    ((WEAPON_INFO *)self)->type = (WEAPON_TYPE)in->as_int;
    return nullptr;
}

// A sample the catalog does not map in this game plays nothing rather than
// misfiring, so anything but a negative id is taken.
static const char *M_SetSample(void *const self, const TRX_VALUE *const in)
{
    if (in->as_int < 0) {
        return "not a sample";
    }
    ((WEAPON_INFO *)self)->sample_num = (SAMPLE_TRX_ID)in->as_int;
    return nullptr;
}

static const char *M_SetOverlaySample(
    void *const self, const TRX_VALUE *const in)
{
    if (in->as_int < 0) {
        return "not a sample";
    }
    ((WEAPON_INFO *)self)->sample_overlay_num = (SAMPLE_TRX_ID)in->as_int;
    return nullptr;
}

// The number counts the animations of the object the weapon is drawn from, and
// the engine switches an item to it without checking. A level that never loaded
// the object has no count to measure against, and nothing to draw either.
static const char *M_SetEquipAnim(void *const self, const TRX_VALUE *const in)
{
    if (in->as_int < 0) {
        return "invalid animation index";
    }
    const LARA_GUN_TYPE gun_type = M_WeaponOfAnim(self)->gun_type;
    const OBJECT_ID object_id = Gun_GetWeaponAnim(gun_type);
    if (object_id != NO_OBJECT) {
        const OBJECT *const obj = Object_Get(object_id);
        if (obj->loaded && in->as_int >= obj->anim_count) {
            return "invalid animation index";
        }
    }
    ((WEAPON_ANIM_INFO *)self)->equip_anim_idx = (int16_t)in->as_int;
    return nullptr;
}

static void *M_ResolveWeapon(const LUA_STRUCT_REF *const ref)
{
    if (ref->handle.id <= LGT_UNARMED
        || !Gun_Registry_IsValidType(ref->handle.id)) {
        return nullptr;
    }
    return Gun_Registry_Get(ref->handle.id);
}

static void *M_ResolveLimits(const LUA_STRUCT_REF *const ref)
{
    WEAPON_INFO *const weapon = M_ResolveWeapon(ref);
    if (weapon == nullptr) {
        return nullptr;
    }
    switch ((M_LIMITS_KIND)ref->handle.gen) {
    case M_LIMITS_LOCK:
        return &weapon->lock;
    case M_LIMITS_LEFT_ARM:
        return &weapon->left_arm;
    case M_LIMITS_RIGHT_ARM:
        return &weapon->right_arm;
    }
    return nullptr;
}

static void *M_ResolveHandPos(const LUA_STRUCT_REF *const ref)
{
    WEAPON_INFO *const weapon = M_ResolveWeapon(ref);
    if (weapon == nullptr) {
        return nullptr;
    }
    switch ((M_HAND_POS_KIND)ref->handle.gen) {
    case M_HAND_POS_MUZZLE:
        return &weapon->muzzle_pos;
    case M_HAND_POS_SHELL:
        return &weapon->shell_pos;
    case M_HAND_POS_FLASH:
        return &weapon->flash.pos;
    }
    return nullptr;
}

static void *M_ResolveAmmo(const LUA_STRUCT_REF *const ref)
{
    WEAPON_INFO *const weapon = M_ResolveWeapon(ref);
    return weapon != nullptr ? &weapon->ammo : nullptr;
}

static void *M_ResolveFlash(const LUA_STRUCT_REF *const ref)
{
    WEAPON_INFO *const weapon = M_ResolveWeapon(ref);
    return weapon != nullptr ? &weapon->flash : nullptr;
}

static void *M_ResolveGlow(const LUA_STRUCT_REF *const ref)
{
    WEAPON_INFO *const weapon = M_ResolveWeapon(ref);
    return weapon != nullptr ? &weapon->glow : nullptr;
}

static void *M_ResolveAnim(const LUA_STRUCT_REF *const ref)
{
    WEAPON_INFO *const weapon = M_ResolveWeapon(ref);
    return weapon != nullptr ? &weapon->anim : nullptr;
}

// Which weapon the handle on the stack names, for a group reached off it.
static int32_t M_CheckWeaponID(lua_State *const L, const int arg)
{
    return LUA_Struct_CheckRef(L, arg, &TYPE_WEAPON_INFO)->handle.id;
}

static void M_PushGroup(
    lua_State *const L, const TYPE_DESC *const type,
    void *(*const resolve)(const LUA_STRUCT_REF *), const int32_t weapon_id,
    const uint32_t which)
{
    LUA_Struct_Push(
        L, type, resolve, (TRX_HANDLE) { .id = weapon_id, .gen = which });
}

// trxc.weapons.get(weapon) -> WEAPON_INFO handle
static int M_L_WeaponGet(lua_State *const L)
{
    LUA_Struct_Push(
        L, &TYPE_WEAPON_INFO, M_ResolveWeapon,
        (TRX_HANDLE) { .id = M_GetWeapon(L, 1) });
    return 1;
}

// trxc.weapons.is_available(weapon) -> bool
static int M_L_WeaponIsAvailable(lua_State *const L)
{
    lua_pushboolean(L, Gun_Registry_Get(M_GetWeapon(L, 1))->is_available);
    return 1;
}

// trxc.weapons.get_object(weapon) -> object id or nil
static int M_L_WeaponGetObject(lua_State *const L)
{
    const OBJECT_ID object_id = Gun_GetGunObject(M_GetWeapon(L, 1));
    if (object_id == NO_OBJECT) {
        lua_pushnil(L);
    } else {
        lua_pushinteger(L, object_id);
    }
    return 1;
}

// trxc.weapons.get_ammo_object(weapon) -> object id or nil
static int M_L_WeaponGetAmmoObject(lua_State *const L)
{
    const OBJECT_ID object_id = Gun_GetAmmoObject(M_GetWeapon(L, 1));
    if (object_id == NO_OBJECT) {
        lua_pushnil(L);
    } else {
        lua_pushinteger(L, object_id);
    }
    return 1;
}

// trxc.weapons.rounds_per_shot(weapon) -> int
static int M_L_WeaponRoundsPerShot(lua_State *const L)
{
    lua_pushinteger(L, Gun_GetRoundsPerShot(M_GetWeapon(L, 1)));
    return 1;
}

// trxc.weapons.shots_per_box(weapon) -> int
static int M_L_WeaponShotsPerBox(lua_State *const L)
{
    const LARA_GUN_TYPE gun_type = M_GetWeapon(L, 1);
    const int32_t rounds_per_shot = Gun_GetRoundsPerShot(gun_type);
    lua_pushinteger(
        L,
        rounds_per_shot > 0 ? Gun_GetRoundsPerBox(gun_type) / rounds_per_shot
                            : 0);
    return 1;
}

// trxc.weapons.get_lock(weapon) -> WEAPON_AIM_LIMITS handle
static int M_L_WeaponGetLock(lua_State *const L)
{
    M_PushGroup(
        L, &TYPE_WEAPON_AIM_LIMITS, M_ResolveLimits, M_CheckWeaponID(L, 1),
        M_LIMITS_LOCK);
    return 1;
}

// trxc.weapons.get_left_arm(weapon) -> WEAPON_AIM_LIMITS handle
static int M_L_WeaponGetLeftArm(lua_State *const L)
{
    M_PushGroup(
        L, &TYPE_WEAPON_AIM_LIMITS, M_ResolveLimits, M_CheckWeaponID(L, 1),
        M_LIMITS_LEFT_ARM);
    return 1;
}

// trxc.weapons.get_right_arm(weapon) -> WEAPON_AIM_LIMITS handle
static int M_L_WeaponGetRightArm(lua_State *const L)
{
    M_PushGroup(
        L, &TYPE_WEAPON_AIM_LIMITS, M_ResolveLimits, M_CheckWeaponID(L, 1),
        M_LIMITS_RIGHT_ARM);
    return 1;
}

// trxc.weapons.get_muzzle_pos(weapon) -> WEAPON_HAND_POS handle
static int M_L_WeaponGetMuzzlePos(lua_State *const L)
{
    M_PushGroup(
        L, &TYPE_WEAPON_HAND_POS, M_ResolveHandPos, M_CheckWeaponID(L, 1),
        M_HAND_POS_MUZZLE);
    return 1;
}

// trxc.weapons.get_shell_pos(weapon) -> WEAPON_HAND_POS handle
static int M_L_WeaponGetShellPos(lua_State *const L)
{
    M_PushGroup(
        L, &TYPE_WEAPON_HAND_POS, M_ResolveHandPos, M_CheckWeaponID(L, 1),
        M_HAND_POS_SHELL);
    return 1;
}

// trxc.weapons.get_ammo(weapon) -> WEAPON_AMMO_INFO handle
static int M_L_WeaponGetAmmo(lua_State *const L)
{
    M_PushGroup(
        L, &TYPE_WEAPON_AMMO_INFO, M_ResolveAmmo, M_CheckWeaponID(L, 1), 0);
    return 1;
}

// trxc.weapons.get_flash(weapon) -> WEAPON_FLASH_INFO handle
static int M_L_WeaponGetFlash(lua_State *const L)
{
    M_PushGroup(
        L, &TYPE_WEAPON_FLASH_INFO, M_ResolveFlash, M_CheckWeaponID(L, 1), 0);
    return 1;
}

// trxc.weapons.get_glow(weapon) -> WEAPON_GLOW_INFO handle
static int M_L_WeaponGetGlow(lua_State *const L)
{
    M_PushGroup(
        L, &TYPE_WEAPON_GLOW_INFO, M_ResolveGlow, M_CheckWeaponID(L, 1), 0);
    return 1;
}

// trxc.weapons.get_anim(weapon) -> WEAPON_ANIM_INFO handle
static int M_L_WeaponGetAnim(lua_State *const L)
{
    M_PushGroup(
        L, &TYPE_WEAPON_ANIM_INFO, M_ResolveAnim, M_CheckWeaponID(L, 1), 0);
    return 1;
}

// trxc.weapons.get_flash_pos(flash) -> WEAPON_HAND_POS handle
//
// Reached off the flash rather than the weapon, so it takes a flash handle.
static int M_L_WeaponGetFlashPos(lua_State *const L)
{
    const LUA_STRUCT_REF *const ref =
        LUA_Struct_CheckRef(L, 1, &TYPE_WEAPON_FLASH_INFO);
    M_PushGroup(
        L, &TYPE_WEAPON_HAND_POS, M_ResolveHandPos, ref->handle.id,
        M_HAND_POS_FLASH);
    return 1;
}

static const luaL_Reg m_Module[] = {
    { "get", M_L_WeaponGet },
    { "is_available", M_L_WeaponIsAvailable },
    { "get_object", M_L_WeaponGetObject },
    { "get_ammo_object", M_L_WeaponGetAmmoObject },
    { "rounds_per_shot", M_L_WeaponRoundsPerShot },
    { "shots_per_box", M_L_WeaponShotsPerBox },
    { "get_lock", M_L_WeaponGetLock },
    { "get_left_arm", M_L_WeaponGetLeftArm },
    { "get_right_arm", M_L_WeaponGetRightArm },
    { "get_muzzle_pos", M_L_WeaponGetMuzzlePos },
    { "get_shell_pos", M_L_WeaponGetShellPos },
    { "get_ammo", M_L_WeaponGetAmmo },
    { "get_flash", M_L_WeaponGetFlash },
    { "get_glow", M_L_WeaponGetGlow },
    { "get_anim", M_L_WeaponGetAnim },
    { "get_flash_pos", M_L_WeaponGetFlashPos },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    LUA_Struct_Register(L, &TYPE_WEAPON_INFO, nullptr);
    LUA_Struct_Register(L, &TYPE_WEAPON_AIM_LIMITS, nullptr);
    LUA_Struct_Register(L, &TYPE_WEAPON_HAND_POS, nullptr);
    LUA_Struct_Register(L, &TYPE_WEAPON_AMMO_INFO, nullptr);
    LUA_Struct_Register(L, &TYPE_WEAPON_FLASH_INFO, nullptr);
    LUA_Struct_Register(L, &TYPE_WEAPON_GLOW_INFO, nullptr);
    LUA_Struct_Register(L, &TYPE_WEAPON_ANIM_INFO, nullptr);
    LUA_RegisterModule(L, "weapons", m_Module);
}

REGISTER_LUA_CAPI(.create = M_Create)
