// The random surface. The assertions live in random.lua.
//
// There is no fake engine: the real generator is what the module reports. The
// seed is fixed here so that the first case can name the numbers it produces.

#include <harness/lua_surface.h>

#include <trx/game/random.h>

int main(void)
{
    Random_SeedControl(1234);
    const LUA_SURFACE_TEST test = {
        .module = "random",
        .tests = "api/random",
    };
    return LuaSurface_Run(&test);
}
