#include "game/lara.h"
#include "game/lua/common.h"

#include <lauxlib.h>

// item = TRX.Lara.GetItem()
static int M_L_GetLaraItem(lua_State *const L)
{
    const ITEM *const item = Lara_GetItem();
    if (item == nullptr) {
        lua_pushnil(L);
    } else {
        const ITEM **ud = lua_newuserdata(L, sizeof(ITEM *));
        *ud = item;
        luaL_getmetatable(L, "TRX.Items.ITEM");
        lua_setmetatable(L, -2);
    }
    return 1;
}

void LUA_CreateLara(lua_State *const L)
{
    lua_getglobal(L, "TRX");
    lua_newtable(L);
    lua_pushcfunction(L, M_L_GetLaraItem);
    lua_setfield(L, -2, "GetItem");
    lua_setfield(L, -2, "Lara");
    lua_pop(L, 1);
}
