#include <fakes/camera.h>
#include <fakes/console.h>
#include <fakes/game.h>
#include <fakes/items.h>
#include <fakes/lara.h>
#include <fakes/rooms.h>
#include <harness/fake_calls.h>
#include <harness/lua_surface.h>

#include <lauxlib.h>

static int M_FakeReset(lua_State *const L)
{
    FakeCalls_Reset();
    // A level is up unless a test says otherwise: that is where the command has
    // work to do.
    FakeGame_SetCurrentLevel(0);
    FakeItems_PlaceCrystal();
    FakeItems_PlaceScion();
    return 0;
}

static void M_PushFake(lua_State *const L)
{
    FakeConsole_PushLua(L);
    FakeGame_PushLua(L);
    lua_pushinteger(L, FAKE_OBJ_VASE);
    lua_setfield(L, -2, "VASE");
    lua_pushinteger(L, FAKE_OBJ_CRYSTAL);
    lua_setfield(L, -2, "CRYSTAL");
    lua_pushinteger(L, FAKE_OBJ_SCION);
    lua_setfield(L, -2, "SCION");
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .fake_reset = M_FakeReset,
        .module = "console",
        // What the command reaches for, plus the real trx.log, which console
        // reads a level off without requiring it.
        .deps = { "log", "objects", "items", "lara", "rooms", "camera", "game",
                  "math", "argparse", nullptr },
        .script = "teleport",
        .tests = "commands/teleport",
        .push_fake = M_PushFake,
    };
    return LuaSurface_Run(&test);
}
