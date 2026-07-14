// The console surface. The assertions live in src/tests/unit/lua/console.lua;
// this stands up the world they run against.
//
// The log module is loaded alongside it: console.lua takes its levels from
// trx.log.LogLevel, so the two share one enum rather than each carrying a copy.

#include "fake_engine_console.h"
#include "lua_surface.h"

#include <lauxlib.h>

static int M_FakeReset(lua_State *const L)
{
    FakeConsole_Reset();
    return 0;
}

// fake.set_eval_result(result) - what the next Console_Eval hands back.
static int M_FakeSetEvalResult(lua_State *const L)
{
    FakeConsole_SetEvalResult((COMMAND_RESULT)luaL_checkinteger(L, 1));
    return 0;
}

static int M_FakeCalls(lua_State *const L)
{
    lua_newtable(L);
    lua_pushinteger(L, g_FakeConsoleCalls.log_count);
    lua_setfield(L, -2, "log_count");
    lua_pushinteger(L, g_FakeConsoleCalls.last_level);
    lua_setfield(L, -2, "last_level");
    lua_pushstring(L, g_FakeConsoleCalls.last_message);
    lua_setfield(L, -2, "last_message");
    lua_pushinteger(L, g_FakeConsoleCalls.clear_count);
    lua_setfield(L, -2, "clear_count");
    lua_pushinteger(L, g_FakeConsoleCalls.eval_count);
    lua_setfield(L, -2, "eval_count");
    lua_pushstring(L, g_FakeConsoleCalls.last_command);
    lua_setfield(L, -2, "last_command");
    lua_pushboolean(L, g_FakeConsoleCalls.verbose_during_eval);
    lua_setfield(L, -2, "verbose_during_eval");
    lua_pushboolean(L, Console_IsVerbose());
    lua_setfield(L, -2, "verbose_now");
    return 1;
}

static void M_PushFake(lua_State *const L)
{
    lua_pushcfunction(L, M_FakeSetEvalResult);
    lua_setfield(L, -2, "set_eval_result");

    lua_newtable(L);
    lua_pushinteger(L, CR_SUCCESS);
    lua_setfield(L, -2, "SUCCESS");
    lua_pushinteger(L, CR_FAILURE);
    lua_setfield(L, -2, "FAILURE");
    lua_pushinteger(L, CR_BAD_INVOCATION);
    lua_setfield(L, -2, "BAD_INVOCATION");
    lua_setfield(L, -2, "CommandResult");
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "console",
        .deps = { "log", nullptr },
        .tests = "console",
        .push_fake = M_PushFake,
        .fake_reset = M_FakeReset,
        .fake_calls = M_FakeCalls,
    };
    return LuaSurface_Run(&test);
}
