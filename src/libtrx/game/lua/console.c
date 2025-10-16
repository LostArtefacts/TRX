#include "game/console/common.h"
#include "game/console/registry.h"
#include "game/game_string.h"
#include "game/lua/common.h"
#include "log.h"
#include "strings.h"

#include <lauxlib.h>

// trx.console.log(...)
static int M_L_ConsoleLog(lua_State *const L)
{
    int nargs = lua_gettop(L);
    const char *msg = nullptr;
    for (int i = 1; i <= nargs; i++) {
        // Convert any Lua value to a string via tostring()
        lua_getglobal(L, "tostring");
        lua_pushvalue(L, i);
        lua_call(L, 1, 1);
        const char *arg = lua_tostring(L, -1);
        lua_pop(L, 1);
        msg = (i > 1) ? String_FormatStatic("%s, %s", msg, arg)
                      : String_FormatStatic("%s", arg);
    }
    Console_Log("%s", msg);
    return 0;
}

// trx.console.clear()
static int M_L_ConsoleClear(lua_State *const L)
{
    Console_Clear();
    return 0;
}

// trx.console.eval(cmd, { verbose = true })
static int M_L_ConsoleEval(lua_State *const L)
{
    const char *cmd = luaL_checkstring(L, 1);
    bool verbose = false;
    if (lua_gettop(L) >= 2 && lua_istable(L, 2)) {
        lua_getfield(L, 2, "verbose");
        verbose = lua_toboolean(L, -1);
        lua_pop(L, 1);
    }
    const bool old_verbose = Console_IsVerbose();
    Console_SetVerbose(verbose);
    COMMAND_RESULT res = Console_Eval(cmd);
    Console_SetVerbose(old_verbose);
    const char *err;
    switch (res) {
    case CR_BAD_INVOCATION:
        err = "bad invocation";
        break;
    case CR_UNAVAILABLE:
        err = "unavailable";
        break;
    case CR_FAILURE:
        err = "failure";
        break;
    case CR_SUCCESS:
        return 0;
    }
    return luaL_error(L, "console.eval %s: %s", err, cmd);
}

void LUA_CreateConsole(lua_State *const L)
{
    lua_getglobal(L, "trx");
    lua_newtable(L);
    lua_pushcfunction(L, M_L_ConsoleLog);
    lua_setfield(L, -2, "log");
    lua_pushcfunction(L, M_L_ConsoleEval);
    lua_setfield(L, -2, "eval");
    lua_pushcfunction(L, M_L_ConsoleClear);
    lua_setfield(L, -2, "clear");
    lua_setfield(L, -2, "console");
    lua_pop(L, 1);
}
