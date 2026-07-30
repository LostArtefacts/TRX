// The help command, exercised through the console that dispatches it. It reads
// the registry through trx.console.commands, so the assertions in
// help.lua run against a real console.

#include <fakes/console.h>
#include <harness/lua_surface.h>

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
        .deps = { "log", "strings", "locale", "argparse", nullptr },
        .script = "help",
        .tests = "commands/help",
        .push_fake = FakeConsole_PushLua,
        .fake_reset = M_FakeReset,
        .fake_calls = M_FakeCalls,
    };
    return LuaSurface_Run(&test);
}
