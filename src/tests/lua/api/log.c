// The logging surface. The assertions live in log.lua; this
// stands up the world they run against.

#include <fakes/log.h>
#include <harness/lua_surface.h>

static void M_PushFake(lua_State *const L)
{
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "log",
        .tests = "api/log",
        .push_fake = M_PushFake,
    };
    return LuaSurface_Run(&test);
}
