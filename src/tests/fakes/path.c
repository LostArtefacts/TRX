// The places on disk with no disk behind them, so a test drives the real path
// surface with nothing installed.
//
// The parts of a path are worked out for real, because the surface's contract
// rests on them; the roots and the search order stand for what a game would
// hold.

#include <trx/core/strings.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/utils.h>

#include <lauxlib.h>
#include <string.h>

static const char *const m_Roots[] = {
    "trx_dir",         "config_dir", "cache_dir",        "games_dir",
    "screenshots_dir", "saves_dir",  "legacy_saves_dir", nullptr,
};

static const char *const m_Kinds[] = {
    "common_config",
    "level_file",
    nullptr,
};

static int M_L_Root(lua_State *const L)
{
    const char *const name = luaL_checkstring(L, 1);
    for (const char *const *it = m_Roots; *it != nullptr; it++) {
        if (strcmp(*it, name) == 0) {
            lua_pushstring(L, String_FormatStatic("/fake/%s", name));
            return 1;
        }
    }
    return luaL_argerror(
        L, 1, String_FormatStatic("there is no place '%s'", name));
}

static int M_L_Roots(lua_State *const L)
{
    lua_newtable(L);
    int32_t idx = 1;
    for (const char *const *it = m_Roots; *it != nullptr; it++) {
        lua_pushstring(L, *it);
        lua_seti(L, -2, idx++);
    }
    return 1;
}

static int M_L_Kinds(lua_State *const L)
{
    lua_newtable(L);
    int32_t idx = 1;
    for (const char *const *it = m_Kinds; *it != nullptr; it++) {
        lua_pushstring(L, *it);
        lua_seti(L, -2, idx++);
    }
    return 1;
}

// Only one name is there to be found, which is enough to tell a hit from a
// miss.
static int M_L_Resolve(lua_State *const L)
{
    const char *const kind = luaL_checkstring(L, 1);
    const char *const rel = luaL_checkstring(L, 2);
    for (const char *const *it = m_Kinds; *it != nullptr; it++) {
        if (strcmp(*it, kind) != 0) {
            continue;
        }
        if (strcmp(rel, "weapons.json5") != 0) {
            lua_pushnil(L);
            return 1;
        }
        lua_pushstring(L, String_FormatStatic("/fake/games_dir/mod/%s", rel));
        return 1;
    }
    return luaL_argerror(
        L, 1, String_FormatStatic("there is no kind of path '%s'", kind));
}

// A token only ever opens a path here, which is how a script writes one.
static int M_L_Expand(lua_State *const L)
{
    const char *const text = luaL_checkstring(L, 1);
    if (text[0] != '%') {
        lua_pushstring(L, text);
        return 1;
    }
    const char *const close = strchr(text + 1, '%');
    if (close == nullptr) {
        lua_pushstring(L, text);
        return 1;
    }
    for (const char *const *it = m_Roots; *it != nullptr; it++) {
        if (strlen(*it) != (size_t)(close - text - 1)
            || strncmp(*it, text + 1, (size_t)(close - text - 1)) != 0) {
            continue;
        }
        lua_pushstring(L, String_FormatStatic("/fake/%s%s", *it, close + 1));
        return 1;
    }
    lua_pushstring(L, text);
    return 1;
}

// Nothing is on disk, so only the name the resolver finds is there.
static int M_L_Exists(lua_State *const L)
{
    lua_pushboolean(
        L,
        strcmp(luaL_checkstring(L, 1), "/fake/games_dir/mod/weapons.json5")
            == 0);
    return 1;
}

static int M_L_Parent(lua_State *const L)
{
    const char *const path = luaL_checkstring(L, 1);
    const char *const slash = strrchr(path, '/');
    lua_pushlstring(L, path, slash != nullptr ? (size_t)(slash - path) : 0);
    return 1;
}

static const char *M_BaseName(const char *const path)
{
    const char *const slash = strrchr(path, '/');
    return slash != nullptr ? slash + 1 : path;
}

static int M_L_Name(lua_State *const L)
{
    lua_pushstring(L, M_BaseName(luaL_checkstring(L, 1)));
    return 1;
}

static int M_L_Stem(lua_State *const L)
{
    const char *const name = M_BaseName(luaL_checkstring(L, 1));
    const char *const dot = strrchr(name, '.');
    lua_pushlstring(
        L, name, dot != nullptr ? (size_t)(dot - name) : strlen(name));
    return 1;
}

static const luaL_Reg m_Module[] = {
    { "root", M_L_Root },     { "roots", M_L_Roots },
    { "kinds", M_L_Kinds },   { "resolve", M_L_Resolve },
    { "expand", M_L_Expand }, { "exists", M_L_Exists },
    { "parent", M_L_Parent }, { "name", M_L_Name },
    { "stem", M_L_Stem },     { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    LUA_RegisterModule(L, "path", m_Module);
}

REGISTER_LUA_CAPI(.create = M_Create)
