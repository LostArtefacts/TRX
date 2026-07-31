#include <fakes/console.h>
#include <fakes/game.h>
#include <fakes/items.h>
#include <fakes/lara.h>
#include <harness/fake_calls.h>
#include <harness/lua_surface.h>

#include <lauxlib.h>

static int M_FakeReset(lua_State *const L)
{
    FakeCalls_Reset();
    // A level is up unless a test says otherwise: that is where the command has
    // work to do.
    FakeGame_SetCurrentLevel(0);
    // The scion's carried state goes into the scion's backpack entry.
    FakeLara_ShareInvEntry(FAKE_OBJ_SCION_2, FAKE_OBJ_SCION);
    // A part-full waterskin goes into the empty one's entry. The fake carries
    // no empty waterskin, so the crowbar stands in for it: what the test needs
    // is a tool with a second state sharing its entry.
    FakeLara_ShareInvEntry(FAKE_OBJ_WATERSKIN, FAKE_OBJ_TOOL);
    return 0;
}

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
    FakeConsole_PushLua(L);
    FakeGame_PushLua(L);
    lua_pushcfunction(L, M_FakeSetCanAdd);
    lua_setfield(L, -2, "set_can_add");
    lua_pushcfunction(L, M_FakeSetWeaponAvailable);
    lua_setfield(L, -2, "set_weapon_available");
    lua_pushinteger(L, FAKE_OBJ_KEY);
    lua_setfield(L, -2, "KEY");
    lua_pushinteger(L, FAKE_OBJ_VASE);
    lua_setfield(L, -2, "VASE");
    lua_pushinteger(L, FAKE_OBJ_SCION);
    lua_setfield(L, -2, "SCION");
    lua_pushinteger(L, FAKE_OBJ_SCION_2);
    lua_setfield(L, -2, "SCION_2");
    lua_pushinteger(L, FAKE_OBJ_CRYSTAL);
    lua_setfield(L, -2, "CRYSTAL");
    lua_pushinteger(L, FAKE_OBJ_TRINKET);
    lua_setfield(L, -2, "TRINKET");
    lua_pushinteger(L, FAKE_OBJ_REAL_KEY);
    lua_setfield(L, -2, "STORY_KEY");
    lua_pushinteger(L, FAKE_OBJ_PUZZLE);
    lua_setfield(L, -2, "PUZZLE");
    lua_pushinteger(L, FAKE_OBJ_TOOL);
    lua_setfield(L, -2, "TOOL");
    lua_pushinteger(L, FAKE_OBJ_LEADBAR);
    lua_setfield(L, -2, "LEADBAR");
    lua_pushinteger(L, FAKE_OBJ_WATERSKIN);
    lua_setfield(L, -2, "WATERSKIN");
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .fake_reset = M_FakeReset,
        .module = "console",
        // What the command reaches for, plus the real trx.log, which console
        // reads a level off without requiring it. What these require comes
        // with them.
        .deps = { "log", "objects", "lara", "game", "sound", "argparse",
                  "inventory", "weapons", nullptr },
        .script = "give",
        .tests = "commands/give",
        .push_fake = M_PushFake,
    };
    return LuaSurface_Run(&test);
}
