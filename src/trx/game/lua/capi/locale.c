// The localized text the player sees. Named locale rather than strings: TRX
// already has two other things called that - trx/core/strings, which
// manipulates them, and trx/game/game_strings, which is this - and a third
// would shadow Lua's own string library besides.

#include <trx/core/memory.h>
#include <trx/core/result.h>
#include <trx/core/vector.h>
#include <trx/game/game_strings/entries.h>
#include <trx/game/game_strings/manager.h>
#include <trx/game/locale/common.h>
#include <trx/game/lua/common.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/utils/module.h>

#include <lauxlib.h>

// trxc.locale.get(key: string): string?
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

// trxc.locale.declare(key: string, text: string)
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

// trxc.locale.reload(): boolean
static int M_L_LocaleReload(lua_State *const L)
{
    lua_pushboolean(
        L,
        Result_Absorb(GameStringManager_ReloadLanguage(
            Locale_GetCode(LOCALE_ROLE_TEXT))));
    return 1;
}

static int M_PushAvailable(lua_State *const L, const LOCALE_ROLE role)
{
    VECTOR *const codes = Locale_GetAvailable(role);
    lua_createtable(L, codes->count, 0);
    for (int32_t i = 0; i < codes->count; i++) {
        char *const code = *(char **)Vector_Get(codes, i);
        lua_pushstring(L, code);
        lua_rawseti(L, -2, i + 1);
        Memory_Free(code);
    }
    Vector_Free(codes);
    return 1;
}

// trxc.locale.text_languages(): string[]
static int M_L_LocaleTextLanguages(lua_State *const L)
{
    return M_PushAvailable(L, LOCALE_ROLE_TEXT);
}

// trxc.locale.voice_languages(): string[]
static int M_L_LocaleVoiceLanguages(lua_State *const L)
{
    return M_PushAvailable(L, LOCALE_ROLE_AUDIO);
}

// trxc.locale.language_name(code: string): string?
static int M_L_LocaleLanguageName(lua_State *const L)
{
    const char *const name = Locale_GetLanguageName(luaL_checkstring(L, 1));
    if (name == nullptr) {
        lua_pushnil(L);
    } else {
        lua_pushstring(L, name);
    }
    return 1;
}

static const luaL_Reg m_Module[] = {
    { "declare", M_L_LocaleDeclare },
    { "get", M_L_LocaleGet },
    { "language_name", M_L_LocaleLanguageName },
    { "reload", M_L_LocaleReload },
    { "text_languages", M_L_LocaleTextLanguages },
    { "voice_languages", M_L_LocaleVoiceLanguages },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    LUA_RegisterModule(L, "locale", m_Module);
}

REGISTER_LUA_CAPI(.create = M_Create)
