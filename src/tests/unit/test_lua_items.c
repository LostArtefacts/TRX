// The item surface. The assertions live in src/tests/unit/lua/items.lua; this
// stands up the world they run against.

#include "fake_engine_items.h"
#include "lua_surface.h"

static void M_SetUpExtra(lua_State *const L)
{
    // items.lua's `room` extension hands off to trx.rooms, which is not under
    // test here.
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
    lua_pushinteger(L, g_FakeItemCalls.kill);
    lua_setfield(L, -2, "kill");
    lua_pushinteger(L, g_FakeItemCalls.creature_die);
    lua_setfield(L, -2, "creature_die");
    lua_pushboolean(L, g_FakeItemCalls.creature_die_explode);
    lua_setfield(L, -2, "creature_die_explode");
    lua_pushinteger(L, g_FakeItemCalls.enable_baddie_ai);
    lua_setfield(L, -2, "enable_baddie_ai");
    return 1;
}

static void M_PushFake(lua_State *const L)
{
    lua_pushinteger(L, FAKE_OBJ_WOLF);
    lua_setfield(L, -2, "WOLF");
    lua_pushinteger(L, FAKE_OBJ_VASE);
    lua_setfield(L, -2, "VASE");
    lua_pushinteger(L, FAKE_OBJ_UNLOADED);
    lua_setfield(L, -2, "UNLOADED");
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "items",
        .tests = "items",
        .setup_extra = M_SetUpExtra,
        .push_fake = M_PushFake,
        .fake_reset = M_FakeReset,
        .fake_calls = M_FakeCalls,
    };
    return LuaSurface_Run(&test);
}
