// The surface after the two boot steps the other suites stop short of: sealing,
// and hardening the globals. The assertions live in
// boot.lua.
//
// It runs against trx.items because a module with checked parameters is what
// makes strict mode worth turning on after hardening.

#include <fakes/items.h>
#include <harness/lua_surface.h>

static void M_SetUpExtra(lua_State *const L)
{
    // As in lua/api/items.c: the `room` extension hands off to trx.rooms.
    (void)luaL_dostring(
        L,
        "trx.rooms = setmetatable({}, {\n"
        "  __index = function(_, n) return { num = n } end })\n");
}

static int M_FakeReset(lua_State *const L)
{
    FakeItems_Reset();
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
        .module = "items",
        .deps = { "query", nullptr },
        .tests = "boot",
        .seal = true,
        .harden = true,
        .setup_extra = M_SetUpExtra,
        .fake_reset = M_FakeReset,
        .fake_calls = M_FakeCalls,
    };
    return LuaSurface_Run(&test);
}
