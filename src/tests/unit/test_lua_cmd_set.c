// The /set command, exercised through the console that dispatches it. The
// assertions live in src/tests/unit/lua/cmd_set.lua; the option map is the
// config fake's, and the fuzzy matching underneath is the real thing.

#include "fake_engine_config.h"
#include "fake_engine_console.h"
#include "lua_surface.h"

#include <lauxlib.h>

static int M_FakeReset(lua_State *const L)
{
    FakeConsole_Reset();
    FakeConfig_Reset();
    return 0;
}

static int M_FakeCalls(lua_State *const L)
{
    lua_newtable(L);
    FakeConsole_PushCalls(L);
    return 1;
}

static int M_FakeSetEnforced(lua_State *const L)
{
    FakeConfig_SetEnforced(lua_toboolean(L, 1));
    return 0;
}

static void M_PushFake(lua_State *const L)
{
    FakeConsole_PushLua(L);
    lua_pushcfunction(L, M_FakeSetEnforced);
    lua_setfield(L, -2, "set_enforced");
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "console",
        // config requires locale, so locale loads first.
        .deps = { "log", "locale", "strings", "config", nullptr },
        .script = "set",
        .tests = "cmd_set",
        .push_fake = M_PushFake,
        .fake_reset = M_FakeReset,
        .fake_calls = M_FakeCalls,
    };
    return LuaSurface_Run(&test);
}
