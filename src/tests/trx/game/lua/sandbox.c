#include <harness/harness.h>

#include <trx/game/lua/sandbox.h>

#include <lauxlib.h>
#include <lualib.h>

// The wall a level script runs behind, stood up against a bare state.

static lua_State *M_Sandboxed(void)
{
    lua_State *const L = luaL_newstate();
    LUA_OpenSafeLibs(L);
    LUA_HardenGlobals(L);
    return L;
}

// luaL_dostring is the C loader; hardening only removes the Lua-visible load().
static bool M_Holds(lua_State *const L, const char *const expr)
{
    char src[256];
    snprintf(src, sizeof(src), "return (%s) and true or false", expr);
    if (luaL_dostring(L, src) != LUA_OK) {
        lua_pop(L, 1);
        return false;
    }
    const bool result = lua_toboolean(L, -1);
    lua_pop(L, 1);
    return result;
}

TEST(sandbox_drops_the_unsafe_libraries)
{
    lua_State *const L = M_Sandboxed();
    // debug.getupvalue on any trx.* function would recover the raw C bridge.
    CHECK(M_Holds(L, "debug == nil"));
    CHECK(M_Holds(L, "os == nil"));
    CHECK(M_Holds(L, "io == nil"));
    lua_close(L);
}

TEST(sandbox_drops_the_base_library_escapes)
{
    lua_State *const L = M_Sandboxed();
    // The base library brings these, so dropping io and os does not close them.
    CHECK(M_Holds(L, "load == nil"));
    CHECK(M_Holds(L, "loadfile == nil"));
    CHECK(M_Holds(L, "dofile == nil"));
    CHECK(M_Holds(L, "string.dump == nil"));
    lua_close(L);
}

TEST(sandbox_drops_the_module_loader)
{
    lua_State *const L = M_Sandboxed();
    CHECK(M_Holds(L, "require == nil"));
    CHECK(M_Holds(L, "package == nil"));
    lua_close(L);
}

TEST(sandbox_locks_the_shared_string_metatable)
{
    lua_State *const L = M_Sandboxed();
    // Left writable, a script rewrites string behaviour for the whole state.
    CHECK(M_Holds(L, "getmetatable('') == 'locked'"));
    lua_close(L);
}

TEST(sandbox_keeps_what_a_script_legitimately_needs)
{
    lua_State *const L = M_Sandboxed();
    CHECK(M_Holds(L, "type(math) == 'table'"));
    CHECK(M_Holds(L, "type(string) == 'table'"));
    CHECK(M_Holds(L, "type(table) == 'table'"));
    CHECK(M_Holds(L, "type(pcall) == 'function'"));
    lua_close(L);
}
