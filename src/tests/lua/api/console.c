// The console surface. The assertions live in console.lua;
// this stands up the world they run against.
//
// The log module is loaded alongside it: console.lua takes its levels from
// trx.log.LogLevel, so the two share one enum rather than each carrying a copy.

#include <fakes/console.h>
#include <harness/lua_surface.h>

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "console",
        .deps = { "log", "strings", "locale", "argparse", nullptr },
        .tests = "api/console",
        .push_fake = FakeConsole_PushLua,
    };
    return LuaSurface_Run(&test);
}
