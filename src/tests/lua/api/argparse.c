// The argument parser surface. The assertions live in
// argparse.lua; this stands up the world they run against.
//
// The matcher argparse leans on is the real one out of trx.strings, so what
// these assert is what the console does when a player types an argument.

#include <harness/lua_surface.h>

static void M_PushFake(lua_State *const L)
{
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "argparse",
        .deps = { "strings", "locale", nullptr },
        .tests = "api/argparse",
        .push_fake = M_PushFake,
    };
    return LuaSurface_Run(&test);
}
