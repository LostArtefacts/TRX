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
    lua_setfield(L, -2, "lara");
    lua_pop(L, 1);
}
