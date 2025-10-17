#include "game/lara.h"
#include "game/lua/common.h"

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

void LUA_CreateLara(lua_State *const L)
{
    lua_getglobal(L, "trxc");
    lua_newtable(L);
    lua_pushcfunction(L, M_L_GetLaraItem);
    lua_setfield(L, -2, "get_item");
    lua_setfield(L, -2, "lara");
    lua_pop(L, 1);
}
