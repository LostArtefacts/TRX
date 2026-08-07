// The weather surface. The assertions live in weather.lua; the world they run
// against is fakes/weather.c.

#include <harness/lua_surface.h>

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "weather",
        .tests = "api/weather",
        .seal = true,
    };
    return LuaSurface_Run(&test);
}
