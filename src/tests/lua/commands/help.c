// The help command, exercised through the console that dispatches it. It reads
// the registry through trx.console.commands, so the assertions in
// help.lua run against a real console.

#include <fakes/console.h>
#include <harness/lua_surface.h>

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "console",
        .deps = { "log", "strings", "locale", "argparse", nullptr },
        .script = "help",
        .tests = "commands/help",
        .push_fake = FakeConsole_PushLua,
    };
    return LuaSurface_Run(&test);
}
