#include <trx/game/lua/api.h>

#include <lauxlib.h>

// Sealing the surface is C's to do once the modules have loaded. So api.lua
// hands the seal to C, and it is held in the Lua registry - reachable by name
// from nowhere a script can see.
static const char m_EntrypointsKey[] = "trx.api.entrypoints";

void LUA_API_SetEntrypoint(
    lua_State *const L, const char *const name, const int fn_idx)
{
    const int fn = lua_absindex(L, fn_idx);
    if (lua_getfield(L, LUA_REGISTRYINDEX, m_EntrypointsKey) != LUA_TTABLE) {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_setfield(L, LUA_REGISTRYINDEX, m_EntrypointsKey);
    }
    lua_pushvalue(L, fn);
    lua_setfield(L, -2, name);
    lua_pop(L, 1);
}

bool LUA_API_PushEntrypoint(lua_State *const L, const char *const name)
{
    if (lua_getfield(L, LUA_REGISTRYINDEX, m_EntrypointsKey) != LUA_TTABLE) {
        lua_pop(L, 1);
        return false;
    }
    const int type = lua_getfield(L, -1, name);
    lua_remove(L, -2);
    if (type != LUA_TFUNCTION) {
        lua_pop(L, 1);
        return false;
    }
    return true;
}
