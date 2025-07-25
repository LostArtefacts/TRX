#include "game/lua/funcs.h"

#include "game/lara.h"
#include "strings.h"

#include <lauxlib.h>

static int M_L_GetLaraItem(lua_State *l);

static int M_L_GetLaraItem(lua_State *l)
{
    ITEM *item = Lara_GetItem();
    if (item == nullptr) {
        lua_pushnil(l);
    } else {
        ITEM **ud = (ITEM **)lua_newuserdata(l, sizeof(ITEM *));
        *ud = item;
        luaL_getmetatable(l, "TRX.ITEM");
        lua_setmetatable(l, -2);
    }
    return 1;
}

void LUA_CreateFunctions(lua_State *const l)
{
    lua_newtable(l);
    lua_pushcfunction(l, M_L_GetLaraItem);
    lua_setfield(l, -2, "get_item");
    lua_setglobal(l, "lara");
}
