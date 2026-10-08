#include <trx/game/lua/api.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/utils.h>

#include <lauxlib.h>

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

static const luaL_Reg m_Module[] = {
    { "set_entrypoint", M_L_SetEntrypoint },
    { "set_color_ctor", M_L_SetColorCtor },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    LUA_RegisterModule(L, "api", m_Module);
}

REGISTER_LUA_CAPI(.create = M_Create)
