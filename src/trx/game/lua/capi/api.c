#include <trx/game/lua/api.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/utils/module.h>
#include <trx/game/lua/utils/value.h>

#include <lauxlib.h>
#include <string.h>

// trxc.api.set_entrypoint(name: string, fn: function)
static int M_L_SetEntrypoint(lua_State *const L)
{
    const char *const name = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);
    LUA_API_SetEntrypoint(L, name, 2);
    return 0;
}

// trxc.api.set_color_ctor(fn: fun(r: number, g: number, b: number, owner: any, key: any): trx.math.Color)
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

// trxc.api.traceback(err: any): any
//
// A message handler for xpcall. Adds the stack as it stands at the error to a
// string error, so that code which catches an error and raises it again still
// reports where it happened. Returns any other error, and a string that
// already carries a traceback, unchanged.
static int M_L_Traceback(lua_State *const L)
{
    lua_settop(L, 1);
    if (lua_type(L, 1) != LUA_TSTRING
        || strstr(lua_tostring(L, 1), LUA_TRACEBACK_MARKER) != nullptr) {
        return 1;
    }
    luaL_traceback(L, L, lua_tostring(L, 1), 1);
    return 1;
}

static const luaL_Reg m_Module[] = {
    { "set_entrypoint", M_L_SetEntrypoint },
    { "set_color_ctor", M_L_SetColorCtor },
    { "traceback", M_L_Traceback },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    LUA_RegisterModule(L, "api", m_Module);
}

REGISTER_LUA_CAPI(.create = M_Create)
