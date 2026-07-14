// The console surface. The assertions live in src/tests/unit/lua/console.lua;
// this stands up the world they run against.
//
// The log module is loaded alongside it: console.lua takes its levels from
// trx.log.LogLevel, so the two share one enum rather than each carrying a copy.

#include "fake_engine_console.h"
#include "lua_surface.h"

static int M_FakeReset(lua_State *const L)
{
    FakeConsole_Reset();
    return 0;
}

static int M_FakeCalls(lua_State *const L)
{
    lua_newtable(L);
    FakeConsole_PushCalls(L);
    return 1;
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "console",
        .deps = { "log", nullptr },
        .tests = "console",
        .push_fake = FakeConsole_PushLua,
        .fake_reset = M_FakeReset,
        .fake_calls = M_FakeCalls,
    };
    return LuaSurface_Run(&test);
}
