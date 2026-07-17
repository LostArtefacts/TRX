#include <trx/game/lua/guard.h>

#include <trx/debug.h>
#include <trx/game/clock.h>

#include <lauxlib.h>

// VM instructions between budget checks. Coarse on purpose: a stuck script
// reaches the next check within microseconds, and every script pays for the
// clock read.
#define M_CHECK_INTERVAL 100000

static double m_BudgetSec = 0.0;

// Zero until the first check after a heartbeat. The budget measures
// continuous Lua execution, not time since the last frame, so a script
// entered after a long C-side stall is not blamed for it.
static double m_DeadlineSec = 0.0;

static void M_Hook(lua_State *L, lua_Debug *ar);

// coroutine.create makes a thread with no hook of its own, so both creators
// route new threads through the guard. wrap keeps its stock behaviour of
// re-raising a resume error in the caller.
static const char M_COROUTINE_PATCH[] =
    "local hook, create, resume = ...\n"
    "coroutine.create = function(f) return hook(create(f)) end\n"
    "coroutine.wrap = function(f)\n"
    "    local co = hook(create(f))\n"
    "    return function(...)\n"
    "        local r = table.pack(resume(co, ...))\n"
    "        if not r[1] then\n"
    "            error(r[2], 0)\n"
    "        end\n"
    "        return table.unpack(r, 2, r.n)\n"
    "    end\n"
    "end\n";

static void M_Hook(lua_State *const L, lua_Debug *const ar)
{
    const double now = Clock_GetRealTime();
    if (m_DeadlineSec == 0.0) {
        m_DeadlineSec = now + m_BudgetSec;
    }
    if (now < m_DeadlineSec) {
        // A trip may have left this thread checking on every instruction.
        lua_sethook(L, M_Hook, LUA_MASKCOUNT, M_CHECK_INTERVAL);
        return;
    }
    // Check on every instruction until the engine has control again: a script
    // that catches this error with pcall dies again before it can loop.
    lua_sethook(L, M_Hook, LUA_MASKCOUNT, 1);
    // lua_pushfstring's %f prints a lua_Number through "%.14g".
    luaL_error(
        L,
        "script ran for over %f seconds without returning control to the "
        "engine",
        m_BudgetSec);
}

static int M_HookThread(lua_State *const L)
{
    lua_State *const co = lua_tothread(L, 1);
    luaL_argcheck(L, co != nullptr, 1, "thread expected");
    lua_sethook(co, M_Hook, LUA_MASKCOUNT, M_CHECK_INTERVAL);
    lua_settop(L, 1);
    return 1;
}

static void M_GuardCoroutines(lua_State *const L)
{
    const int status = luaL_loadbuffer(
        L, M_COROUTINE_PATCH, sizeof(M_COROUTINE_PATCH) - 1, "@trx-guard");
    ASSERT(status == LUA_OK);
    lua_pushcfunction(L, M_HookThread);
    lua_getglobal(L, "coroutine");
    lua_getfield(L, -1, "create");
    lua_getfield(L, -2, "resume");
    lua_remove(L, -3);
    lua_call(L, 3, 0);
}

void LUA_Guard_Install(lua_State *const L, const double budget_sec)
{
    m_BudgetSec = budget_sec;
    m_DeadlineSec = 0.0;
    lua_sethook(L, M_Hook, LUA_MASKCOUNT, M_CHECK_INTERVAL);
    M_GuardCoroutines(L);
}

// The main loop has control at this moment, so no script is stuck.
void LUA_Guard_Heartbeat(void)
{
    m_DeadlineSec = 0.0;
}
