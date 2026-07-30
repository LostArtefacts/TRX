// The config surface. The assertions live in config.lua;
// this stands up the world they run against.

#include <fakes/config.h>
#include <harness/lua_surface.h>

#include <lauxlib.h>

static int M_FakeReset(lua_State *const L)
{
    FakeConfig_Reset();
    return 0;
}

static int M_FakeSetEnforced(lua_State *const L)
{
    FakeConfig_SetEnforced(lua_toboolean(L, 1));
    return 0;
}

static int M_FakeCalls(lua_State *const L)
{
    lua_newtable(L);
    lua_pushinteger(L, g_FakeConfigCalls.config_writes);
    lua_setfield(L, -2, "config_writes");
    return 1;
}

static void M_PushFake(lua_State *const L)
{
    lua_pushcfunction(L, M_FakeSetEnforced);
    lua_setfield(L, -2, "set_enforced");
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "config",
        .deps = { "locale", nullptr },
        .tests = "api/config",
        .push_fake = M_PushFake,
        .fake_reset = M_FakeReset,
        .fake_calls = M_FakeCalls,
    };
    return LuaSurface_Run(&test);
}
