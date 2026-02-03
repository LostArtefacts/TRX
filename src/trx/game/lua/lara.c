#include <trx/game/gun.h>
#include <trx/game/inventory.h>
#include <trx/game/lara.h>
#include <trx/game/lua/common.h>

#include <lauxlib.h>

// item_num = trxc.lara.get_item()
static int M_L_GetLaraItem(lua_State *const L)
{
    const ITEM *const item = Lara_GetItem();
    int result = 0;
    if (item != nullptr) {
        result = Item_GetIndex(item) + 1;
    }
    if (result == 0) {
        lua_pushnil(L);
    } else {
        lua_pushinteger(L, result);
    }
    return 1;
}

// trxc.lara.get_exposure_bar() → int
static int M_L_LaraGetExposureBar(lua_State *const L)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    lua_pushinteger(L, lara->exposure_timer);
    return 1;
}

// trxc.lara.set_exposure_bar(timer)
static int M_L_LaraSetExposureBar(lua_State *const L)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->exposure_timer = luaL_checkinteger(L, 1);
    return 0;
}

// trxc.lara.get_air_bar() → int
static int M_L_LaraGetAirBar(lua_State *const L)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    lua_pushinteger(L, lara->air);
    return 1;
}

// trxc.lara.set_air_bar(timer)
static int M_L_LaraSetAirBar(lua_State *const L)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->air = luaL_checkinteger(L, 1);
    return 0;
}

// trxc.lara.get_skin() → int
static int M_L_LaraGetSkin(lua_State *const L)
{
    const int32_t skin_idx = Lara_Skin_GetType();
    lua_pushinteger(L, skin_idx);
    return 1;
}

// trxc.lara.set_skin(skin_idx)
static int M_L_LaraSetSkin(lua_State *const L)
{
    const int32_t skin_idx = luaL_checkinteger(L, 1);
    Lara_Skin_SetType(skin_idx);
    return 0;
}

// trxc.lara.set_extra_equipment(lara_mesh, extra_mesh)
static int M_L_LaraSetExtraEquipment(lua_State *const L)
{
    const LARA_MESH lara_mesh = luaL_checkinteger(L, 1);
    const LARA_SKIN_EXTRA_MESH extra_mesh = luaL_checkinteger(L, 2);
    Lara_Skin_SetExtraEquipment(lara_mesh, extra_mesh);
    return 0;
}

// trxc.lara.clear_equipment(lara_mesh)
static int M_L_LaraClearEquipment(lua_State *const L)
{
    const LARA_MESH lara_mesh = luaL_checkinteger(L, 1);
    Lara_Skin_ClearEquipment(lara_mesh);
    return 0;
}

// trxc.lara.are_holsters_visible() → bool
static int M_L_LaraAreHolstersVisible(lua_State *const L)
{
    lua_pushboolean(L, Lara_Skin_AreHolstersVisible());
    return 1;
}

// trxc.lara.set_holsters_visible(visible)
static int M_L_LaraSetHolstersVisible(lua_State *const L)
{
    const bool visible = lua_toboolean(L, 1) != 0;
    Lara_Skin_SetHolstersVisible(visible);
    return 0;
}

// trxc.lara.has_pistol_weapon() → bool
static int M_L_LaraHasPistolWeapon(lua_State *const L)
{
    bool has_pistol = false;
    for (int32_t i = 0; i < NUM_WEAPONS; i++) {
        const WEAPON_INFO *const weapon = &g_Weapons[i];
        if ((weapon->type == WEAPON_TYPE_DUAL_PISTOLS
             || weapon->type == WEAPON_TYPE_SINGLE_PISTOL)
            && Inv_RequestItem(Gun_GetGunObject(i))) {
            has_pistol = true;
            break;
        }
    }
    lua_pushboolean(L, has_pistol);
    return 1;
}

// trxc.lara.get_extra_anim() → int
static int M_L_LaraGetExtraAnim(lua_State *const L)
{
    if (Lara_GetLaraInfo()->extra_anim) {
        lua_pushinteger(
            L, Item_GetRelativeObjAnim(Lara_GetItem(), O_LARA_EXTRA));
    } else {
        lua_pushinteger(L, NO_ANIM);
    }
    return 1;
}

void LUA_CreateLara(lua_State *const L)
{
    lua_getglobal(L, "trxc");
    lua_newtable(L);
    lua_pushcfunction(L, M_L_GetLaraItem);
    lua_setfield(L, -2, "get_item");
    lua_pushcfunction(L, M_L_LaraGetExposureBar);
    lua_setfield(L, -2, "get_exposure_bar");
    lua_pushcfunction(L, M_L_LaraSetExposureBar);
    lua_setfield(L, -2, "set_exposure_bar");
    lua_pushcfunction(L, M_L_LaraGetAirBar);
    lua_setfield(L, -2, "get_air_bar");
    lua_pushcfunction(L, M_L_LaraSetAirBar);
    lua_setfield(L, -2, "set_air_bar");
    lua_pushcfunction(L, M_L_LaraGetSkin);
    lua_setfield(L, -2, "get_skin");
    lua_pushcfunction(L, M_L_LaraSetSkin);
    lua_setfield(L, -2, "set_skin");
    lua_pushcfunction(L, M_L_LaraSetExtraEquipment);
    lua_setfield(L, -2, "set_extra_equipment");
    lua_pushcfunction(L, M_L_LaraClearEquipment);
    lua_setfield(L, -2, "clear_equipment");
    lua_pushcfunction(L, M_L_LaraAreHolstersVisible);
    lua_setfield(L, -2, "are_holsters_visible");
    lua_pushcfunction(L, M_L_LaraSetHolstersVisible);
    lua_setfield(L, -2, "set_holsters_visible");
    lua_pushcfunction(L, M_L_LaraHasPistolWeapon);
    lua_setfield(L, -2, "has_pistol_weapon");
    lua_pushcfunction(L, M_L_LaraGetExtraAnim);
    lua_setfield(L, -2, "get_extra_anim");
    lua_setfield(L, -2, "lara");
    lua_pop(L, 1);
}
