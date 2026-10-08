#include <trx/game/lua/utils/module.h>

void LUA_RegisterModule(
    lua_State *const L, const char *const name, const luaL_Reg *const fns)
{
    lua_getglobal(L, "trxc");
    lua_newtable(L);
    luaL_setfuncs(L, fns, 0);
    lua_setfield(L, -2, name);
    lua_pop(L, 1);
}

void LUA_GetModule(lua_State *const L, const char *const name)
{
    lua_getglobal(L, "trxc");
    lua_getfield(L, -1, name);
    lua_remove(L, -2);
}
