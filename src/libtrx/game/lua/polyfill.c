#include "game/lua/polyfill.h"

int luaL_getsubtable(lua_State *L, int idx, const char *fname)
{
    lua_getfield(L, idx, fname);
    if (lua_istable(L, -1)) {
        return 1;
    }
    lua_pop(L, 1);
    lua_newtable(L);
    lua_pushvalue(L, -1);
    lua_setfield(L, idx, fname);
    return 0;
}
