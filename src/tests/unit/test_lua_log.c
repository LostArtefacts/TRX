// The logging surface. The assertions live in src/tests/unit/lua/log.lua; this
// stands up the world they run against.

#include "fake_engine_log.h"
#include "lua_surface.h"

static int M_FakeReset(lua_State *const L)
{
    FakeLog_Reset();
    return 0;
}

static int M_FakeCalls(lua_State *const L)
{
    lua_newtable(L);
    lua_pushinteger(L, g_FakeLogCalls.count);
    lua_setfield(L, -2, "count");
    lua_pushinteger(L, g_FakeLogCalls.last_level);
    lua_setfield(L, -2, "last_level");
    lua_pushstring(L, g_FakeLogCalls.last_message);
    lua_setfield(L, -2, "last_message");
    lua_pushinteger(L, g_FakeLogCalls.last_line);
    lua_setfield(L, -2, "last_line");
    return 1;
}

static void M_PushFake(lua_State *const L)
{
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "log",
        .tests = "log",
        .push_fake = M_PushFake,
        .fake_reset = M_FakeReset,
        .fake_calls = M_FakeCalls,
    };
    return LuaSurface_Run(&test);
}
