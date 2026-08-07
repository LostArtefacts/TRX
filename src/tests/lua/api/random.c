// The random surface. The assertions live in random.lua.
//
// There is no fake engine: the real generator is what the module reports. The
// seed is fixed once the modules are up - standing them up seeds the generator
// from the clock - so that the first case can name the numbers it produces.

#include <harness/lua_surface.h>

#include <trx/game/random.h>

static void M_Seed(lua_State *const L)
{
    Random_SeedControl(1234);
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "random",
        .tests = "api/random",
        .setup_extra = M_Seed,
    };
    return LuaSurface_Run(&test);
}
