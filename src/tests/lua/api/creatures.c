// The creature surface. The assertions live in
// creatures.lua; this stands up the world they run against.

#include <fakes/creatures.h>
#include <harness/lua_surface.h>

static void M_PushFake(lua_State *const L)
{
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "creatures",
        .tests = "api/creatures",
        .push_fake = M_PushFake,
    };
    return LuaSurface_Run(&test);
}
