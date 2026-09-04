// Reading and writing JSON. The assertions live in json.lua; this stands up
// the world they run against.

#include <harness/lua_surface.h>

static void M_PushFake(lua_State *const L)
{
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "json",
        .deps = { "path" },
        .tests = "api/json",
        .push_fake = M_PushFake,
    };
    return LuaSurface_Run(&test);
}
