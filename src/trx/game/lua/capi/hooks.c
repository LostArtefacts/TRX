#include <trx/game/lua/hooks.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/utils/module.h>

#include <lauxlib.h>

// trxc.hooks.set(name: string, fn?: function, ...: any)
static int M_L_Set(lua_State *const L)
{
    const char *const name = luaL_checkstring(L, 1);
    const LUA_HOOK_TYPE hook = LUA_Hooks_GetByName(name);
    if (hook == LUA_HOOK_NUMBER_OF) {
        return luaL_argerror(
            L, 1, lua_pushfstring(L, "no such hook '%s'", name));
    }
    if (!lua_isnoneornil(L, 2)) {
        luaL_checktype(L, 2, LUA_TFUNCTION);
    }
    LUA_Hooks_Set(L, hook, LUA_Hooks_ReadKey(L, hook, 3), 2);
    return 0;
}

static const luaL_Reg m_Module[] = {
    { "set", M_L_Set },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    LUA_RegisterModule(L, "hooks", m_Module);
}

REGISTER_LUA_CAPI(.create = M_Create)
