// Sealing is a one-way door, so it gets a state of its own. Same world as
// test_lua_items.c; the assertions live in src/tests/unit/lua/api_seal.lua.

#include "fake_engine_items.h"
#include "lua_surface.h"

static void M_SetUpExtra(lua_State *const L)
{
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

static void M_PushFake(lua_State *const L)
{
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "items",
        .deps = { "query", nullptr },
        .tests = "api_seal",
        .setup_extra = M_SetUpExtra,
        .push_fake = M_PushFake,
        .fake_reset = M_FakeReset,
        .fake_calls = M_FakeCalls,
    };
    return LuaSurface_Run(&test);
}
