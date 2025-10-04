#include "game/console/common.h"
#include "game/console/registry.h"
#include "game/game_string.h"
#include "game/lua/common.h"
#include "log.h"
#include "strings.h"

#include <lauxlib.h>

// TRX.Console.Log(...)
static int M_L_ConsoleLog(lua_State *const L)
{
    int nargs = lua_gettop(L);
    const char *msg = NULL;
    for (int i = 1; i <= nargs; i++) {
        const char *arg = lua_tostring(L, i);
        msg = (i > 1) ? String_FormatStatic("%s, %s", msg, arg)
                      : String_FormatStatic("%s", arg);
    }
    Console_Log("%s", msg);
    return 0;
}

// TRX.Console.Clear()
static int M_L_ConsoleClear(lua_State *const L)
{
    Console_Clear();
    return 0;
}

// TRX.Console.Eval(cmd)
static int M_L_ConsoleEval(lua_State *const L)
{
    const char *cmd = luaL_checkstring(L, 1);
    COMMAND_RESULT res = Console_Eval(cmd);
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
    lua_getglobal(L, "TRX");
    lua_newtable(L);
    lua_pushcfunction(L, M_L_ConsoleLog);
    lua_setfield(L, -2, "Log");
    lua_pushcfunction(L, M_L_ConsoleEval);
    lua_setfield(L, -2, "Eval");
    lua_pushcfunction(L, M_L_ConsoleClear);
    lua_setfield(L, -2, "Clear");
    lua_setfield(L, -2, "Console");
    lua_pop(L, 1);
}
