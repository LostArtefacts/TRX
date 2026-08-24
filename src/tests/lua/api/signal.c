// Signal API tests. The config surface provides an engine-backed signal.

#include <harness/lua_surface.h>

#include <trx/game/lua/common.h>
#include <trx/game/lua/events.h>

#include <lauxlib.h>

// fake.as_level_script(fn) - run fn as a level script rather than a global one,
// so the signals it makes are the level's.
static int M_FakeAsLevelScript(lua_State *const L)
{
    luaL_checktype(L, 1, LUA_TFUNCTION);
    LUA_SetScriptContext(LUA_CONTEXT_LEVEL);
    const int status = lua_pcall(L, 0, 0, 0);
    LUA_SetScriptContext(LUA_CONTEXT_GLOBAL);
    if (status != LUA_OK) {
        return lua_error(L);
    }
    return 0;
}

// fake.tick() - one engine tick, as the phase executor raises it.
static int M_FakeTick(lua_State *const L)
{
    LUA_FireEvent(LUA_EVENT_TICK);
    return 0;
}

// fake.end_level() - what the engine does when a level ends, in the order it
// does it: the script hears the unload, and then its listeners go.
static int M_FakeEndLevel(lua_State *const L)
{
    LUA_FireEvent(LUA_EVENT_LEVEL_UNLOAD);
    LUA_ClearLevelListeners();
    return 0;
}

static void M_PushFake(lua_State *const L)
{
    lua_pushcfunction(L, M_FakeAsLevelScript);
    lua_setfield(L, -2, "as_level_script");
    lua_pushcfunction(L, M_FakeEndLevel);
    lua_setfield(L, -2, "end_level");
    lua_pushcfunction(L, M_FakeTick);
    lua_setfield(L, -2, "tick");
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "signal",
        .deps = { "config", "events", nullptr },
        .tests = "api/signal",
        .push_fake = M_PushFake,
    };
    return LuaSurface_Run(&test);
}
