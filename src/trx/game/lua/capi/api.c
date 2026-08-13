#include <trx/game/lua/api.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/utils.h>

#include <lauxlib.h>

// The registry closes itself once the engine's declarations are in, and takes
// its declaring half off trx.api as the seal takes trxc off the globals: what a
// script cannot successfully call, it should not be able to reach.
//
// Sealing and dumping the surface are still C's to do, and the dump runs after
// the seal. So api.lua hands both to C here, and they are held in the Lua
// registry - reachable by name from nowhere a script can see.
static const char m_EntrypointsKey[] = "trx.api.entrypoints";

// trxc.api.set_entrypoint(name, fn)
static int M_L_SetEntrypoint(lua_State *const L)
{
    const char *const name = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    if (lua_getfield(L, LUA_REGISTRYINDEX, m_EntrypointsKey) != LUA_TTABLE) {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_setfield(L, LUA_REGISTRYINDEX, m_EntrypointsKey);
    }
    lua_pushvalue(L, 2);
    lua_setfield(L, -2, name);
    lua_pop(L, 1);
    return 0;
}

// trxc.api.set_color_ctor(fn)
//
// What a color is is declared in trx.math; this is how the bridges get hold of
// it, so that a color read off a struct or a setting comes back as that type
// rather than as a bare table of channels.
static int M_L_SetColorCtor(lua_State *const L)
{
    luaL_checktype(L, 1, LUA_TFUNCTION);
    LUA_SetColorConstructor(L, 1);
    return 0;
}

static const luaL_Reg m_Module[] = {
    { "set_entrypoint", M_L_SetEntrypoint },
    { "set_color_ctor", M_L_SetColorCtor },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    LUA_RegisterModule(L, "api", m_Module);
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

REGISTER_LUA_CAPI(.create = M_Create)
