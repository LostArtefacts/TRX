// The fixed-point trig surface. The assertions live in
// math.lua.
//
// There is no fake engine: the real trig tables are what the module is for, so
// they are what it is tested against.

#include <harness/lua_surface.h>

static int M_FakeReset(lua_State *const L)
{
    return 0;
}

static int M_FakeCalls(lua_State *const L)
{
    lua_newtable(L);
    return 1;
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "math",
        .tests = "api/math",
        .fake_reset = M_FakeReset,
        .fake_calls = M_FakeCalls,
    };
    return LuaSurface_Run(&test);
}
