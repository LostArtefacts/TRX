#include <trx/core/enum_map.h>
#include <trx/core/log.h>
#include <trx/core/math/const.h>
#include <trx/core/strings.h>
#include <trx/core/utils.h>
#include <trx/core/vector.h>
#include <trx/game/catalog/manager.h>
#include <trx/game/console.h>
#include <trx/game/const.h>
#include <trx/game/gun.h>
#include <trx/game/gun/common.h>
#include <trx/game/gun/registry.h>
#include <trx/game/gun/routines.h>
#include <trx/game/lua/capi/weapons_spec.h>
#include <trx/game/lua/common.h>
#include <trx/game/lua/field.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/struct.h>
#include <trx/game/lua/utils.h>
#include <trx/game/objects.h>
#include <trx/game/objects/ids.h>
#include <trx/game/objects/names.h>

#include <lauxlib.h>
#include <string.h>

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

typedef struct {
    LARA_GUN_TYPE gun_type;
    int32_t fire_ref;
} M_SCRIPT_WEAPON;

static VECTOR *m_ScriptWeapons = nullptr;
static lua_State *m_L = nullptr;

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

    FIELD(WEAPON_INFO, aim_speed),
    FIELD(WEAPON_INFO, shot_accuracy),
    FIELD(WEAPON_INFO, gun_height),
    FIELD(WEAPON_INFO, damage),
    FIELD(WEAPON_INFO, target_dist),
    FIELD(WEAPON_INFO, smoke_count),
    FIELD_SET(WEAPON_INFO, sample_num, M_SetSample),
    FIELD_SET(WEAPON_INFO, sample_overlay_num, M_SetOverlaySample),
    FIELD(WEAPON_INFO, sample_overlay_pitch),

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
    if (!Gun_Registry_IsValidType(gun_type)) {
        luaL_argerror(
            L, arg,
            String_FormatStatic("there is no weapon %d", (int32_t)gun_type));
    }
    if (gun_type <= LGT_UNARMED) {
        luaL_argerror(
            L, arg,
            String_FormatStatic(
                "'%s' is empty hands rather than a weapon",
                Catalog_IDToKey(CATALOG_WEAPONS, gun_type)));
    }
    return (LARA_GUN_TYPE)gun_type;
}

static const WEAPON_INFO *M_WeaponOfAnim(const void *const anim)
{
    return (const WEAPON_INFO *)((const char *)anim
                                 - offsetof(WEAPON_INFO, anim));
}

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
    if (!IGNORE(Gun_Registry_SetKind((WEAPON_TYPE)in->as_int, self))) {
        return "the engine holds no weapon of that kind";
    }
    return nullptr;
}

static const char *M_SetSample(void *const self, const TRX_VALUE *const in)
{
    if (in->as_int < 0) {
        return "not a sample";
    }
    ((WEAPON_INFO *)self)->sample_num = (SAMPLE_ID)in->as_int;
    return nullptr;
}

static const char *M_SetOverlaySample(
    void *const self, const TRX_VALUE *const in)
{
    if (in->as_int < 0) {
        return "not a sample";
    }
    ((WEAPON_INFO *)self)->sample_overlay_num = (SAMPLE_ID)in->as_int;
    return nullptr;
}

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

static M_SCRIPT_WEAPON *M_FindScriptWeapon(const LARA_GUN_TYPE gun_type)
{
    for (int32_t i = 0;
         m_ScriptWeapons != nullptr && i < m_ScriptWeapons->count; i++) {
        M_SCRIPT_WEAPON *const entry = Vector_Get(m_ScriptWeapons, i);
        if (entry->gun_type == gun_type) {
            return entry;
        }
    }
    return nullptr;
}

static void M_Fire(const LARA_GUN_TYPE gun_type, const bool running)
{
    const M_SCRIPT_WEAPON *const entry = M_FindScriptWeapon(gun_type);
    if (entry == nullptr || m_L == nullptr) {
        return;
    }
    lua_rawgeti(m_L, LUA_REGISTRYINDEX, entry->fire_ref);
    LUA_Struct_Push(
        m_L, &TYPE_WEAPON_INFO, M_ResolveWeapon,
        (TRX_HANDLE) { .id = gun_type });
    lua_pushboolean(m_L, running);
    if (lua_pcall(m_L, 2, 0, 0) != LUA_OK) {
        Console_ShowError("weapon fire error: %s", lua_tostring(m_L, -1));
        lua_pop(m_L, 1);
    }
}

static void M_ReadSpec(
    lua_State *const L, const int idx, const LARA_GUN_TYPE gun_type)
{
    RESULT result = LUA_Weapons_ReadSpec(L, idx, Gun_Registry_Get(gun_type));
    if (!IS_OK(result)) {
        lua_pushstring(L, result.msg != nullptr ? result.msg : "bad spec");
        IGNORE(result);
        lua_error(L);
    }
}

// The weapon the argument names, or NO_CATALOG_ID for a name the catalog does
// not hold yet. A name nothing holds is refused unless the caller mints it.
static LARA_GUN_TYPE M_ResolveTarget(
    lua_State *const L, const int arg, const bool may_mint)
{
    if (lua_type(L, arg) == LUA_TSTRING) {
        const char *const key = lua_tostring(L, arg);
        const CATALOG_ID found =
            Catalog_KeyToID(CATALOG_WEAPONS, key, NO_CATALOG_ID);
        if (found == NO_CATALOG_ID && !may_mint) {
            luaL_error(L, "there is no weapon '%s'", key);
        }
        return (LARA_GUN_TYPE)found;
    }

    const LARA_GUN_TYPE gun_type = luaL_checkinteger(L, arg);
    if (!Gun_Registry_IsValidType(gun_type)) {
        luaL_argerror(
            L, arg,
            String_FormatStatic("there is no weapon %d", (int32_t)gun_type));
    }
    if (gun_type <= LGT_UNARMED) {
        luaL_argerror(
            L, arg,
            String_FormatStatic(
                "'%s' is empty hands rather than a weapon",
                Catalog_IDToKey(CATALOG_WEAPONS, gun_type)));
    }
    return gun_type;
}

// Whether a spec says what the weapon is, which a weapon of its own needs
// before anything else the spec holds means something.
static bool M_SpecNamesKind(lua_State *const L, const int idx)
{
    static const char *const keys[] = { "kind", "base" };
    bool named = false;
    for (int32_t i = 0; i < (int32_t)ARRAY_SIZE(keys); i++) {
        lua_getfield(L, idx, keys[i]);
        named = named || !lua_isnoneornil(L, -1);
        lua_pop(L, 1);
    }
    return named;
}

// trxc.weapons.declare(name or id, spec) -> weapon
static int M_L_WeaponDeclare(lua_State *const L)
{
    LARA_GUN_TYPE gun_type = M_ResolveTarget(L, 1, true);
    const char *const key =
        gun_type == NO_CATALOG_ID ? lua_tostring(L, 1) : nullptr;
    luaL_checktype(L, 2, LUA_TTABLE);
    if (gun_type != NO_CATALOG_ID && Gun_Registry_Get(gun_type)->is_claimed) {
        luaL_error(
            L, "'%s' is a weapon already; patch it to change it",
            Catalog_IDToKey(CATALOG_WEAPONS, gun_type));
    }
    if (!M_SpecNamesKind(L, 2)) {
        luaL_error(
            L, "'%s' says neither a kind nor a base, so it is no weapon yet",
            key != nullptr ? key : Catalog_IDToKey(CATALOG_WEAPONS, gun_type));
    }

    if (gun_type == NO_CATALOG_ID) {
        CATALOG_ID minted = NO_CATALOG_ID;
        RESULT result = Catalog_CreateKey(CATALOG_WEAPONS, key, &minted);
        if (!IS_OK(result)) {
            lua_pushstring(
                L, result.msg != nullptr ? result.msg : "cannot be minted");
            IGNORE(result);
            lua_error(L);
        }
        gun_type = (LARA_GUN_TYPE)minted;
    }
    M_ReadSpec(L, 2, gun_type);
    LUA_Struct_Push(
        L, &TYPE_WEAPON_INFO, M_ResolveWeapon, (TRX_HANDLE) { .id = gun_type });
    return 1;
}

// trxc.weapons.patch(name or id, spec) -> weapon
static int M_L_WeaponPatch(lua_State *const L)
{
    const LARA_GUN_TYPE gun_type = M_ResolveTarget(L, 1, false);
    luaL_checktype(L, 2, LUA_TTABLE);
    M_ReadSpec(L, 2, gun_type);
    LUA_Struct_Push(
        L, &TYPE_WEAPON_INFO, M_ResolveWeapon, (TRX_HANDLE) { .id = gun_type });
    return 1;
}

// trxc.weapons.set_fire(gun_type, handler)
static int M_L_WeaponSetFire(lua_State *const L)
{
    const LARA_GUN_TYPE gun_type = M_ResolveTarget(L, 1, false);
    luaL_checktype(L, 2, LUA_TFUNCTION);
    LUA_Weapons_SetFireHandler(L, gun_type, 2);
    return 0;
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

// trxc.weapons.ammo_icon(weapon) -> string or nil
//
// Returns the TR1 ammunition-count icon markup.
static int M_L_WeaponAmmoIcon(lua_State *const L)
{
    const char *const icon = Gun_Registry_Get(M_GetWeapon(L, 1))->ammo_icon;
    if (icon == nullptr) {
        lua_pushnil(L);
    } else {
        lua_pushstring(L, icon);
    }
    return 1;
}

// trxc.weapons.has_infinite_ammo(weapon) -> bool
static int M_L_WeaponHasInfiniteAmmo(lua_State *const L)
{
    lua_pushboolean(L, Gun_HasInfiniteAmmo(M_GetWeapon(L, 1)));
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
    { "declare", M_L_WeaponDeclare },
    { "patch", M_L_WeaponPatch },
    { "set_fire", M_L_WeaponSetFire },
    { "is_available", M_L_WeaponIsAvailable },
    { "get_object", M_L_WeaponGetObject },
    { "get_ammo_object", M_L_WeaponGetAmmoObject },
    { "rounds_per_shot", M_L_WeaponRoundsPerShot },
    { "ammo_icon", M_L_WeaponAmmoIcon },
    { "has_infinite_ammo", M_L_WeaponHasInfiniteAmmo },
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

static void M_Shutdown(void)
{
    for (int32_t i = 0;
         m_ScriptWeapons != nullptr && i < m_ScriptWeapons->count; i++) {
        const M_SCRIPT_WEAPON *const entry = Vector_Get(m_ScriptWeapons, i);
        if (m_L != nullptr) {
            luaL_unref(m_L, LUA_REGISTRYINDEX, entry->fire_ref);
        }
    }
    if (m_ScriptWeapons != nullptr) {
        Vector_Free(m_ScriptWeapons);
        m_ScriptWeapons = nullptr;
    }
    m_L = nullptr;
}

static void M_Create(lua_State *const L)
{
    m_L = L;
    LUA_Struct_Register(L, &TYPE_WEAPON_INFO, nullptr);
    LUA_Struct_Register(L, &TYPE_WEAPON_AIM_LIMITS, nullptr);
    LUA_Struct_Register(L, &TYPE_WEAPON_HAND_POS, nullptr);
    LUA_Struct_Register(L, &TYPE_WEAPON_AMMO_INFO, nullptr);
    LUA_Struct_Register(L, &TYPE_WEAPON_FLASH_INFO, nullptr);
    LUA_Struct_Register(L, &TYPE_WEAPON_GLOW_INFO, nullptr);
    LUA_Struct_Register(L, &TYPE_WEAPON_ANIM_INFO, nullptr);
    LUA_RegisterModule(L, "weapons", m_Module);
}

void LUA_Weapons_SetFireHandler(
    lua_State *const L, const LARA_GUN_TYPE gun_type, const int idx)
{
    M_SCRIPT_WEAPON *entry = M_FindScriptWeapon(gun_type);
    if (entry != nullptr) {
        luaL_unref(L, LUA_REGISTRYINDEX, entry->fire_ref);
    }
    lua_pushvalue(L, idx);
    const int32_t ref = luaL_ref(L, LUA_REGISTRYINDEX);
    if (entry == nullptr) {
        if (m_ScriptWeapons == nullptr) {
            m_ScriptWeapons = Vector_Create(sizeof(M_SCRIPT_WEAPON));
        }
        const M_SCRIPT_WEAPON added = {
            .gun_type = gun_type,
            .fire_ref = ref,
        };
        Vector_Add(m_ScriptWeapons, &added);
    } else {
        entry->fire_ref = ref;
    }
    Gun_Registry_Get(gun_type)->fire_func = M_Fire;
}

REGISTER_LUA_CAPI(.create = M_Create, .shutdown = M_Shutdown)
