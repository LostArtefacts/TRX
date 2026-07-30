#include <fakes/console.h>
#include <fakes/game.h>
#include <fakes/stats.h>
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

static int M_FakeSetSecrets(lua_State *const L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    const int32_t count = (int32_t)lua_rawlen(L, 1);

    int32_t nums[16];
    luaL_argcheck(
        L, count <= (int32_t)(sizeof(nums) / sizeof(nums[0])), 1, "too many");
    for (int32_t i = 0; i < count; i++) {
        lua_rawgeti(L, 1, i + 1);
        nums[i] = (int32_t)luaL_checkinteger(L, -1);
        lua_pop(L, 1);
    }

    FakeStats_SetSecrets(nums, count);
    return 0;
}

static int M_FakeSetFound(lua_State *const L)
{
    FakeStats_SetFound((int32_t)luaL_checkinteger(L, 1), lua_toboolean(L, 2));
    return 0;
}

static void M_PushFake(lua_State *const L)
{
    FakeConsole_PushLua(L);
    FakeGame_PushLua(L);
    lua_pushcfunction(L, M_FakeSetSecrets);
    lua_setfield(L, -2, "set_secrets");
    lua_pushcfunction(L, M_FakeSetFound);
    lua_setfield(L, -2, "set_found");
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .fake_reset = M_FakeReset,
        .module = "console",
        .deps = { "log", "strings", "game", "stats", "locale", "argparse",
                  nullptr },
        .script = "secret",
        .tests = "commands/secret",
        .push_fake = M_PushFake,
    };
    return LuaSurface_Run(&test);
}
