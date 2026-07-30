// The /set command, exercised through the console that dispatches it. The
// assertions live in set.lua; the option map is the
// config fake's, and the fuzzy matching underneath is the real thing.

#include <fakes/config.h>
#include <fakes/console.h>
#include <harness/lua_surface.h>

#include <lauxlib.h>

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
        .deps = { "log", "locale", "strings", "config", "argparse", nullptr },
        .script = "set",
        .tests = "commands/set",
        .push_fake = M_PushFake,
    };
    return LuaSurface_Run(&test);
}
