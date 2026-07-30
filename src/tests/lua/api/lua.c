// The eval surface. The assertions live in lua.lua; the
// fake runs the evaluated chunks on the test's own state, so what is under
// test is the shape of the answer, with real successes and real failures
// behind it.

#include <fakes/lua.h>
#include <harness/lua_surface.h>

static void M_PushFake(lua_State *const L)
{
    FakeLua_SetState(L);
    lua_pushstring(L, REPO_ROOT "/src/tests/lua/fixtures/");
    lua_setfield(L, -2, "script_dir");
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "lua",
        .tests = "api/lua",
        .push_fake = M_PushFake,
    };
    return LuaSurface_Run(&test);
}
