#include <trx/game/game_strings/entries.h>
#include <trx/game/lua/common.h>

#include <lauxlib.h>

// The localized text the player sees. Named locale rather than strings: TRX
// already has two other things called that - trx/core/strings, which
// manipulates them, and trx/game/game_strings, which is this - and a third
// would shadow Lua's own string library besides.

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

void LUA_CreateLocale(lua_State *const L)
{
    lua_getglobal(L, "trxc");
    lua_newtable(L);
    lua_pushcfunction(L, M_L_LocaleGet);
    lua_setfield(L, -2, "get");
    lua_setfield(L, -2, "locale");
    lua_pop(L, 1);
}
