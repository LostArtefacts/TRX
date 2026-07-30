// The /rule command, exercised through the console that dispatches it. The
// assertions live in rule.lua; the rule map is the real
// one, and the fuzzy matching underneath is the real thing.

#include <fakes/console.h>
#include <harness/lua_surface.h>

#include <trx/game/rules.h>

#include <lauxlib.h>

// Rules_Reset reads the version for the rules whose default depends on it.
int32_t g_TRVersion = 1;

static int M_FakeReset(lua_State *const L)
{
    FakeConsole_Reset();
    // Rules are engine state, not the console's, so the harness has to put
    // them back itself for each case to start from the same world.
    Rules_Reset();
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
        .deps = { "log", "locale", "strings", "rules", "argparse", nullptr },
        .script = "rule",
        .tests = "commands/rule",
        .push_fake = FakeConsole_PushLua,
        .fake_reset = M_FakeReset,
        .fake_calls = M_FakeCalls,
    };
    return LuaSurface_Run(&test);
}
