#include <trx/core/enum_map.h>
#include <trx/core/filesystem.h>
#include <trx/core/memory.h>
#include <trx/core/strings.h>
#include <trx/core/vector.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/utils.h>
#include <trx/game/paths.h>

#include <lauxlib.h>

static GAME_PATH M_CheckRoot(lua_State *const L, const int arg)
{
    const char *const name = luaL_checkstring(L, arg);
    const int32_t found = ENUM_MAP_GET(GAME_PATH, name, -1);
    if (found < 0) {
        luaL_argerror(
            L, arg, String_FormatStatic("there is no place '%s'", name));
    }
    return (GAME_PATH)found;
}

// trxc.path.root(name) -> the directory the name stands for
static int M_L_Root(lua_State *const L)
{
    const char *const dir = GamePath_Get(M_CheckRoot(L, 1));
    if (dir == nullptr) {
        lua_pushnil(L);
        return 1;
    }
    lua_pushstring(L, dir);
    return 1;
}

// trxc.path.roots() -> every name a place is known by
static int M_L_Roots(lua_State *const L)
{
    lua_newtable(L);
    int32_t idx = 1;
#define M_DIR_NAME(name, field, token)                                         \
    lua_pushstring(L, #field);                                                 \
    lua_seti(L, -2, idx++);
    GAME_PATH_DIR_LIST(M_DIR_NAME)
#undef M_DIR_NAME
    return 1;
}

// trxc.path.kinds() -> every kind of file that may be resolved
static int M_L_Kinds(lua_State *const L)
{
    VECTOR *const names = EnumMap_ListValues(ENUM_MAP_NAME(GAME_DYNAMIC_PATH));
    lua_newtable(L);
    for (int32_t i = 0; names != nullptr && i < names->count; i++) {
        lua_pushstring(L, *(const char **)Vector_Get(names, i));
        lua_seti(L, -2, i + 1);
    }
    if (names != nullptr) {
        Vector_Free(names);
    }
    return 1;
}

// trxc.path.resolve(kind, rel) -> where the file is, or nil for none
static int M_L_Resolve(lua_State *const L)
{
    const char *const name = luaL_checkstring(L, 1);
    const char *const rel = luaL_checkstring(L, 2);
    const int32_t kind = ENUM_MAP_GET(GAME_DYNAMIC_PATH, name, -1);
    if (kind < 0) {
        return luaL_argerror(
            L, 1, String_FormatStatic("there is no kind of path '%s'", name));
    }
    const char *const path = GamePath_PeekResolve((GAME_DYNAMIC_PATH)kind, rel);
    if (path == nullptr) {
        lua_pushnil(L);
        return 1;
    }
    lua_pushstring(L, path);
    return 1;
}

// trxc.path.expand(text) -> the text with every %token% filled in
static int M_L_Expand(lua_State *const L)
{
    char *const out = GamePath_ExpandVars(luaL_checkstring(L, 1));
    lua_pushstring(L, out != nullptr ? out : "");
    Memory_Free(out);
    return 1;
}

// trxc.path.exists(raw) -> whether anything is there
static int M_L_Exists(lua_State *const L)
{
    lua_pushboolean(L, FS_Exists(luaL_checkstring(L, 1)));
    return 1;
}

// trxc.path.parent(raw) -> the directory the path sits in
static int M_L_Parent(lua_State *const L)
{
    char *const out = FS_GetParentDirectory(luaL_checkstring(L, 1));
    lua_pushstring(L, out != nullptr ? out : "");
    Memory_Free(out);
    return 1;
}

// trxc.path.name(raw) -> the name at the end of the path
static int M_L_Name(lua_State *const L)
{
    const char *const out = FS_GetBaseName(luaL_checkstring(L, 1));
    lua_pushstring(L, out != nullptr ? out : "");
    return 1;
}

// trxc.path.stem(raw) -> the name at the end, without its extension
static int M_L_Stem(lua_State *const L)
{
    char *const out = FS_GetStem(luaL_checkstring(L, 1));
    lua_pushstring(L, out != nullptr ? out : "");
    Memory_Free(out);
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
