// The weapon definitions a script reads and writes. The assertions live in
// weapons.lua.
//
// The weapon table itself is the engine's own - it is a plain array of
// definitions - so what the fake stands in for is the rest of the gun code the
// bridges ask about: which pickup a weapon is, and what it is drawn from.

#include <fakes/lara.h>
#include <harness/lua_surface.h>

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "weapons",
        .deps = { "catalog", "math", nullptr },
        .tests = "api/weapons",
    };
    return LuaSurface_Run(&test);
}
