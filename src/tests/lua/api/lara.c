// The Lara surface. The assertions live in lara.lua; this
// stands up the world they run against.
//
// trx.lara is a module standing for one C struct, so a read of trx.lara.air
// goes through the reflection layer into a real LARA_INFO. That is the thing
// being tested; only what is not a field of it is faked.

#include <fakes/items.h>
#include <fakes/lara.h>
#include <harness/lua_surface.h>

#include <lauxlib.h>

// fake.set_can_add(bool) - whether the level carries the inventory models.
static int M_FakeSetCanAdd(lua_State *const L)
{
    FakeLara_SetCanAdd(lua_toboolean(L, 1));
    return 0;
}

static int M_FakeSetWeaponAvailable(lua_State *const L)
{
    FakeLara_SetWeaponAvailable(
        (LARA_GUN_TYPE)luaL_checkinteger(L, 1), lua_toboolean(L, 2));
    return 0;
}

static void M_PushFake(lua_State *const L)
{
    lua_pushcfunction(L, M_FakeSetCanAdd);
    lua_setfield(L, -2, "set_can_add");
    lua_pushcfunction(L, M_FakeSetWeaponAvailable);
    lua_setfield(L, -2, "set_weapon_available");
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "lara",
        .deps = { "query", "items", "catalog", nullptr },
        .tests = "api/lara",
        .push_fake = M_PushFake,
    };
    return LuaSurface_Run(&test);
}
