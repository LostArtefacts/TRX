#include <trx/game/gun.h>
#include <trx/game/lua/common.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/utils.h>

#include <lauxlib.h>

// What a weapon is, rather than what anyone has of it. None of this differs
// between the inventory Lara carries and the one a level keeps for her, which
// is why it belongs to neither.

static LARA_GUN_TYPE M_GetWeapon(lua_State *const L, const int arg)
{
    const lua_Integer gun_type = luaL_checkinteger(L, arg);
    if (gun_type <= LGT_UNARMED || gun_type >= NUM_WEAPONS) {
        luaL_argerror(L, arg, "not a weapon");
    }
    return (LARA_GUN_TYPE)gun_type;
}

// trxc.weapons.is_available(weapon) -> bool
static int M_L_WeaponIsAvailable(lua_State *const L)
{
    lua_pushboolean(L, g_Weapons[M_GetWeapon(L, 1)].is_available);
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

// trxc.weapons.shots_per_box(weapon) -> int
static int M_L_WeaponShotsPerBox(lua_State *const L)
{
    const LARA_GUN_TYPE gun_type = M_GetWeapon(L, 1);
    lua_pushinteger(
        L, Gun_GetRoundsPerBox(gun_type) / Gun_GetRoundsPerShot(gun_type));
    return 1;
}

static const luaL_Reg m_Module[] = {
    { "is_available", M_L_WeaponIsAvailable },
    { "get_object", M_L_WeaponGetObject },
    { "shots_per_box", M_L_WeaponShotsPerBox },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    LUA_RegisterModule(L, "weapons", m_Module);
}

REGISTER_LUA_CAPI(.create = M_Create)
