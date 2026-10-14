// The game flow surface. The assertions live in game.lua;
// this stands up the world they run against.

#include <fakes/game.h>
#include <harness/lua_surface.h>

#include <trx/game/game_flow.h>
#include <trx/game/lua/common.h>
#include <trx/game/lua/events.h>
#include <trx/game/lua/hooks.h>
#include <trx/game/lua/registry.h>

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

// fake.as_level_script(fn) - runs fn as a level script.
static int M_L_AsLevelScript(lua_State *const L)
{
    luaL_checktype(L, 1, LUA_TFUNCTION);
    LUA_SetScriptContext(LUA_CONTEXT_LEVEL);
    const int status = lua_pcall(L, 0, 0, 0);
    LUA_SetScriptContext(LUA_CONTEXT_GLOBAL);
    if (status != LUA_OK) {
        return lua_error(L);
    }
    return 0;
}

// fake.end_level() - the unload event, and then the level's listeners go.
static int M_L_EndLevel(lua_State *const L)
{
    LUA_FireEvent(LUA_EVENT_LEVEL_UNLOAD);
    LUA_Registry_DropLevelAll();
    return 0;
}

// fake.tick()
static int M_L_Tick(lua_State *const L)
{
    LUA_Tick();
    return 0;
}

static void M_PushFake(lua_State *const L)
{
    FakeGame_PushLua(L);
    lua_pushcfunction(L, M_L_AsLevelScript);
    lua_setfield(L, -2, "as_level_script");
    lua_pushcfunction(L, M_L_EndLevel);
    lua_setfield(L, -2, "end_level");
    lua_pushcfunction(L, M_L_Tick);
    lua_setfield(L, -2, "tick");
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
