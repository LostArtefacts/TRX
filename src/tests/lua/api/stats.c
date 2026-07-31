// The statistics surface. The assertions live in stats.lua;
// this stands up the world they run against.

#include <fakes/game.h>
#include <fakes/stats.h>
#include <harness/lua_surface.h>

#include <lauxlib.h>

// fake.set_secrets({ 1, 3, 7 }) - the numbers the level's scan reserved, none
// of them found yet.
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

static int M_FakeSetMaxSecretCount(lua_State *const L)
{
    FakeStats_SetMaxSecretCount((int32_t)luaL_checkinteger(L, 1));
    return 0;
}

// fake.set_count(level_num, category, n) - what Lara has of one thing in a
// level, which need not be the one being played.
static int M_FakeSetCount(lua_State *const L)
{
    FakeStats_SetCount(
        (int32_t)luaL_checkinteger(L, 1), (int32_t)luaL_checkinteger(L, 2),
        (int32_t)luaL_checkinteger(L, 3));
    return 0;
}

static int M_FakeSetMax(lua_State *const L)
{
    FakeStats_SetMax(
        (int32_t)luaL_checkinteger(L, 1), (int32_t)luaL_checkinteger(L, 2),
        (int32_t)luaL_checkinteger(L, 3));
    return 0;
}

static int M_FakeSetUnobtainable(lua_State *const L)
{
    FakeStats_SetUnobtainable(
        (int32_t)luaL_checkinteger(L, 1), (int32_t)luaL_checkinteger(L, 2),
        (int32_t)luaL_checkinteger(L, 3));
    return 0;
}

// fake.set_kill_split(level_num, allies, enemies) - how the level's kill
// maximum divides, which is what the statistics screen adjusts.
static int M_FakeSetKillSplit(lua_State *const L)
{
    FakeStats_SetKillSplit(
        (int32_t)luaL_checkinteger(L, 1), (int32_t)luaL_checkinteger(L, 2),
        (int32_t)luaL_checkinteger(L, 3));
    return 0;
}

static int M_FakeSetAlliesHurt(lua_State *const L)
{
    FakeStats_SetAlliesHurt(
        (int32_t)luaL_checkinteger(L, 1), lua_toboolean(L, 2));
    return 0;
}

static void M_PushFake(lua_State *const L)
{
    FakeGame_PushLua(L);
    lua_pushcfunction(L, M_FakeSetSecrets);
    lua_setfield(L, -2, "set_secrets");
    lua_pushcfunction(L, M_FakeSetFound);
    lua_setfield(L, -2, "set_found");
    lua_pushcfunction(L, M_FakeSetMaxSecretCount);
    lua_setfield(L, -2, "set_max_secret_count");
    lua_pushcfunction(L, M_FakeSetCount);
    lua_setfield(L, -2, "set_count");
    lua_pushcfunction(L, M_FakeSetMax);
    lua_setfield(L, -2, "set_max");
    lua_pushcfunction(L, M_FakeSetUnobtainable);
    lua_setfield(L, -2, "set_unobtainable");
    lua_pushcfunction(L, M_FakeSetKillSplit);
    lua_setfield(L, -2, "set_kill_split");
    lua_pushcfunction(L, M_FakeSetAlliesHurt);
    lua_setfield(L, -2, "set_allies_hurt");
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "stats",
        // A level hands back its statistics, which is how a script reaches a
        // level other than the one being played.
        .deps = { "game" },
        .tests = "api/stats",
        .push_fake = M_PushFake,
    };
    return LuaSurface_Run(&test);
}
