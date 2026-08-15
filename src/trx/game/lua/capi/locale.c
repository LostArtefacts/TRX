// The localized text the player sees. Named locale rather than strings: TRX
// already has two other things called that - trx/core/strings, which
// manipulates them, and trx/game/game_strings, which is this - and a third
// would shadow Lua's own string library besides.

#include <trx/config.h>
#include <trx/core/result.h>
#include <trx/game/game_strings/entries.h>
#include <trx/game/game_strings/manager.h>
#include <trx/game/lua/common.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/utils.h>

#include <lauxlib.h>

// trxc.locale.get(key) -> string or nil
static int M_L_LocaleGet(lua_State *const L)
{
    const char *const value = GameString_Get(luaL_checkstring(L, 1));
    if (value == nullptr) {
        lua_pushnil(L);
    } else {
        lua_pushstring(L, value);
    }
    return 1;
}

// trxc.locale.declare(key, text)
static int M_L_LocaleDeclare(lua_State *const L)
{
    // A declaration is a fallback, and the strings files run before the level
    // scripts do: taking a key the table already holds would put English over
    // the player's own language.
    const char *const key = luaL_checkstring(L, 1);
    const char *const text = luaL_checkstring(L, 2);
    if (!GameString_IsKnown(key)) {
        GameString_Define(key, text);
    }
    return 0;
}

// trxc.locale.reload() -> bool
static int M_L_LocaleReload(lua_State *const L)
{
    lua_pushboolean(
        L, Result_Absorb(GameStringManager_ReloadLanguage(g_Config.language)));
    return 1;
}

static const luaL_Reg m_Module[] = {
    { "declare", M_L_LocaleDeclare },
    { "get", M_L_LocaleGet },
    { "reload", M_L_LocaleReload },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    LUA_RegisterModule(L, "locale", m_Module);
}

REGISTER_LUA_CAPI(.create = M_Create)
