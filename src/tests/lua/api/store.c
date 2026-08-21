// The persistent store surface. The assertions live in store.lua; this stands
// up the round trip through a savegame that they run against.

#include <harness/lua_surface.h>

#include <trx/core/json/base.h>
#include <trx/game/lua/store.h>

#include <lauxlib.h>

// fake.round_trip() -> bool
//
// Writes both stores as a save does and reads them straight back, which is the
// whole of what a savegame does to them.
static int M_FakeRoundTrip(lua_State *const L)
{
    JSON_WRITE_IO *const writer = JSON_WriteIO_Create();
    LUA_Store_Dump(writer);
    JSON_VALUE *const root = JSON_ValueCopy(JSON_WriteIO_GetRoot(writer));
    JSON_WriteIO_Destroy(writer);

    JSON_READ_IO *const reader = JSON_ReadIO_Create(root, 0, "store test");
    const RESULT result = LUA_Store_Load(reader);
    JSON_ReadIO_Destroy(reader);
    JSON_ValueFree(root);

    const bool ok = IS_OK(result);
    IGNORE(result);
    lua_pushboolean(L, ok);
    return 1;
}

// fake.clear_level()
static int M_FakeClearLevel(lua_State *const L)
{
    LUA_Store_ClearLevel();
    return 0;
}

// fake.clear_game()
static int M_FakeClearGame(lua_State *const L)
{
    LUA_Store_ClearGame();
    return 0;
}

static void M_PushFake(lua_State *const L)
{
    lua_pushcfunction(L, M_FakeRoundTrip);
    lua_setfield(L, -2, "round_trip");
    lua_pushcfunction(L, M_FakeClearLevel);
    lua_setfield(L, -2, "clear_level");
    lua_pushcfunction(L, M_FakeClearGame);
    lua_setfield(L, -2, "clear_game");
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "store",
        .tests = "api/store",
        .push_fake = M_PushFake,
    };
    return LuaSurface_Run(&test);
}
