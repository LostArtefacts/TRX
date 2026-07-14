// The surface after the two boot steps the other suites stop short of: sealing,
// and hardening the globals. The assertions live in
// src/tests/unit/lua/boot.lua.
//
// It runs against trx.items because a module with checked parameters is what
// makes strict mode worth turning on after hardening.

#include "fake_engine_items.h"
#include "lua_surface.h"

static void M_SetUpExtra(lua_State *const L)
{
    // As in test_lua_items: the `room` extension hands off to trx.rooms.
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
        .tests = "boot",
        .seal = true,
        .harden = true,
        .setup_extra = M_SetUpExtra,
        .fake_reset = M_FakeReset,
        .fake_calls = M_FakeCalls,
    };
    return LuaSurface_Run(&test);
}
