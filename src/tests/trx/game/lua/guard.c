#include <harness/harness.h>

#include <trx/game/lua/guard.h>

#include <lauxlib.h>
#include <lualib.h>

// The guard reads the clock only from its hook, so a stepping stub stands in
// for real time: every read moves it 10 ms forward, and a script that loops
// long enough to be checked a few hundred times runs out of budget.

static double m_Now = 0.0;

static lua_State *M_Guarded(void)
{
    lua_State *const L = luaL_newstate();
    luaL_openlibs(L);
    LUA_Guard_Install(L, 3.0);
    return L;
}

static bool M_Dies(lua_State *const L, const char *const src)
{
    LUA_Guard_Heartbeat();
    if (luaL_dostring(L, src) == LUA_OK) {
        return false;
    }
    const char *const message = lua_tostring(L, -1);
    const bool blamed = message != nullptr
        && strstr(message, "without returning control") != nullptr;
    lua_pop(L, 1);
    return blamed;
}

double Clock_GetRealTime(void)
{
    m_Now += 0.01;
    return m_Now;
}

TEST(guard_aborts_a_runaway_script)
{
    lua_State *const L = M_Guarded();
    CHECK(M_Dies(L, "while true do end"));
    lua_close(L);
}

TEST(guard_outlives_a_pcall_that_catches_it)
{
    lua_State *const L = M_Guarded();
    CHECK(
        M_Dies(L, "while true do pcall(function() while true do end end) end"));
    lua_close(L);
}

TEST(guard_covers_coroutines)
{
    lua_State *const L = M_Guarded();
    CHECK(M_Dies(L, "coroutine.wrap(function() while true do end end)()"));
    CHECK(M_Dies(
        L,
        "local co = coroutine.create(function() while true do end end)\n"
        "local ok, err = coroutine.resume(co)\n"
        "assert(not ok)\n"
        "error(err, 0)"));
    lua_close(L);
}

TEST(guard_heartbeat_resets_the_budget)
{
    lua_State *const L = M_Guarded();
    CHECK(M_Dies(L, "while true do end"));
    // The heartbeat in M_Dies starts a fresh budget, so a script that finishes
    // in time is untouched - even right after a trip.
    LUA_Guard_Heartbeat();
    CHECK(luaL_dostring(L, "return 1 + 1") == LUA_OK);
    lua_close(L);
}

TEST(guard_ignores_time_the_engine_had_control)
{
    lua_State *const L = M_Guarded();
    const char *const work = "for i = 1, 300000 do end";
    LUA_Guard_Heartbeat();
    CHECK(luaL_dostring(L, work) == LUA_OK);
    // A long engine-side stall between two calls into Lua, with no heartbeat
    // in between, as during start-up.
    m_Now += 100.0;
    CHECK(luaL_dostring(L, work) == LUA_OK);
    lua_close(L);
}
