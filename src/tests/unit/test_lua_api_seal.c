// Sealing is a one-way door, so it gets a state of its own. Same world as
// test_lua_items.c; the assertions live in src/tests/unit/lua/api_seal.lua.

#include "fake_engine_items.h"
#include "lua_surface.h"

extern void LUA_CreateItems(lua_State *L);

static void M_SetUpTRXC(lua_State *const L)
{
    LUA_CreateItems(L);
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
        .tests = "api_seal",
        .setup_trxc = M_SetUpTRXC,
        .push_fake = M_PushFake,
        .fake_reset = M_FakeReset,
        .fake_calls = M_FakeCalls,
    };
    return LuaSurface_Run(&test);
}
