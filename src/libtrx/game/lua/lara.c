#include "game/lara.h"
#include "game/lua/common.h"

#include <lauxlib.h>

// item = trx.lara.get_item()
static int M_L_GetLaraItem(lua_State *const L)
{
    const ITEM *const item = Lara_GetItem();
    if (item == nullptr) {
        lua_pushnil(L);
    } else {
        const ITEM **ud = lua_newuserdata(L, sizeof(ITEM *));
        *ud = item;
        luaL_getmetatable(L, "trx.items.Item");
        lua_setmetatable(L, -2);
    }
    return 1;
}

void LUA_CreateLara(lua_State *const L)
{
    lua_getglobal(L, "trx");
    lua_newtable(L);
    lua_pushcfunction(L, M_L_GetLaraItem);
    lua_setfield(L, -2, "get_item");
    lua_setfield(L, -2, "lara");
    lua_pop(L, 1);
}
