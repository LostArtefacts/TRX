// Exercises the copy command through console dispatch.

#include <fakes/console.h>
#include <harness/lua_surface.h>

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "console",
        .deps = { "log", "strings", "locale", "argparse", nullptr },
        .script = "copy",
        .tests = "commands/copy",
        .push_fake = FakeConsole_PushLua,
    };
    return LuaSurface_Run(&test);
}
