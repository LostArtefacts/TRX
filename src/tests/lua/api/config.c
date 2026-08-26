// The config surface. The assertions live in config.lua;
// this stands up the world they run against.

#include <fakes/config.h>
#include <fakes/level.h>
#include <harness/lua_surface.h>

#include <trx/game/lua/common.h>

#include <lauxlib.h>

static int M_FakeSetEnforced(lua_State *const L)
{
    FakeConfig_SetEnforced(lua_toboolean(L, 1));
    return 0;
}

// What a level script's own registrations are made under, and what ends them.
static int M_FakeAsLevelScript(lua_State *const L)
{
    LUA_SetScriptContext(
        lua_toboolean(L, 1) ? LUA_CONTEXT_LEVEL : LUA_CONTEXT_GLOBAL);
    return 0;
}

// Whether the level has been read yet. A level script runs before it has.
static int M_FakeSetWorldLoaded(lua_State *const L)
{
    FakeLevel_SetWorldLoaded(lua_toboolean(L, 1));
    return 0;
}

// What Level_Initialise does once the level is read.
static int M_FakeLoadWorld(lua_State *const L)
{
    FakeLevel_SetWorldLoaded(true);
    LUA_Config_FlushPendingWatchers();
    return 0;
}

static int M_FakeEndLevel(lua_State *const L)
{
    LUA_Config_ClearLevelWatchers();
    return 0;
}

static void M_PushFake(lua_State *const L)
{
    lua_pushcfunction(L, M_FakeSetEnforced);
    lua_setfield(L, -2, "set_enforced");
    lua_pushcfunction(L, M_FakeAsLevelScript);
    lua_setfield(L, -2, "as_level_script");
    lua_pushcfunction(L, M_FakeEndLevel);
    lua_setfield(L, -2, "end_level");
    lua_pushcfunction(L, M_FakeSetWorldLoaded);
    lua_setfield(L, -2, "set_world_loaded");
    lua_pushcfunction(L, M_FakeLoadWorld);
    lua_setfield(L, -2, "load_world");
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "config",
        .deps = { "locale", "math", nullptr },
        .tests = "api/config",
        .push_fake = M_PushFake,
    };
    return LuaSurface_Run(&test);
}
