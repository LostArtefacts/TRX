// The fixed-point trig surface. The assertions live in
// math.lua.
//
// There is no fake engine: the real trig tables are what the module is for, so
// they are what it is tested against.

#include <harness/lua_surface.h>

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "math",
        .tests = "api/math",
    };
    return LuaSurface_Run(&test);
}
