// The game flow surface. The assertions live in game.lua;
// this stands up the world they run against.

#include <fakes/game.h>
#include <harness/lua_surface.h>

#include <trx/game/game_flow.h>
#include <trx/game/lua/hooks.h>

#include <lauxlib.h>

// fake.check_bonus(num, unlocked) - asks the bonus check about a level of
// the main table, as the game flow does when the level before it ends.
static int M_L_CheckBonus(lua_State *const L)
{
    const GF_LEVEL *const level =
        GF_GetLevelByOrdinalNumber(GFLT_MAIN, (int32_t)luaL_checkinteger(L, 1));
    const bool unlocked = lua_toboolean(L, 2);
    lua_pushboolean(
        L,
        LUA_Hooks_CallBool(
            LUA_HOOK_BONUS_CHECK, 0, unlocked, LUA_ARG_LEVEL(level),
            LUA_ARG_BOOL(unlocked)));
    return 1;
}

static void M_PushFake(lua_State *const L)
{
    FakeGame_PushLua(L);
    lua_pushcfunction(L, M_L_CheckBonus);
    lua_setfield(L, -2, "check_bonus");
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "game",
        .tests = "trx/game",
        .push_fake = M_PushFake,
    };
    return LuaSurface_Run(&test);
}
