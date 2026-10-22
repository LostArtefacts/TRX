#include <fakes/console.h>
#include <fakes/game.h>
#include <fakes/items.h>
#include <harness/fake_calls.h>
#include <harness/lua_surface.h>

static int M_FakeReset(lua_State *const L)
{
    FakeCalls_Reset();
    // A level is up unless a test says otherwise: that is where the command has
    // work to do.
    FakeGame_SetCurrentLevel(0);
    return 0;
}

static int M_FakePlaceEnemyJeep(lua_State *const L)
{
    FakeItems_PlaceEnemyJeep();
    return 0;
}

static void M_PushFake(lua_State *const L)
{
    FakeConsole_PushLua(L);
    FakeGame_PushLua(L);
    lua_pushcfunction(L, M_FakePlaceEnemyJeep);
    lua_setfield(L, -2, "place_enemy_jeep");
    lua_pushinteger(L, FAKE_OBJ_WOLF);
    lua_setfield(L, -2, "WOLF");
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .fake_reset = M_FakeReset,
        .module = "console",
        .deps = { "log", "query", "items", "objects", "creatures", "lara",
                  "game", "sound", "locale", "argparse", nullptr },
        .script = "kill",
        .tests = "commands/kill",
        .push_fake = M_PushFake,
    };
    return LuaSurface_Run(&test);
}
