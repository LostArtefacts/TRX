#include "game/lua/funcs.h"

#include "game/console/common.h"
#include "game/lara.h"
#include "strings.h"

#include <lauxlib.h>

static int M_L_GetLaraItem(lua_State *l)
{
    ITEM *item = Lara_GetItem();
    if (item == nullptr) {
        lua_pushnil(l);
    } else {
        ITEM **ud = (ITEM **)lua_newuserdata(l, sizeof(ITEM *));
        *ud = item;
        luaL_getmetatable(l, "TRX.ITEM");
        lua_setmetatable(l, -2);
    }
    return 1;
}

static int M_L_ConsoleLog(lua_State *l)
{
    const int32_t n = lua_gettop(l);
    const char *log_message;
    for (int32_t i = 1; i <= n; i++) {
        const char *const arg = lua_tostring(l, i);
        if (i > 1) {
            log_message = String_FormatStatic("%s, %s", log_message, arg);
        } else {
            log_message = String_FormatStatic("%s", arg);
        }
    }
    Console_Log("%s", log_message);
    return 0;
}

static int M_L_ConsoleEval(lua_State *l)
{
    const char *cmd = luaL_checkstring(l, 1);
    COMMAND_RESULT result = Console_Eval(cmd);
    if (result != CR_SUCCESS) {
        switch (result) {
        case CR_BAD_INVOCATION:
            return luaL_error(l, "console.eval bad invocation: %s", cmd);
        case CR_UNAVAILABLE:
            return luaL_error(l, "console.eval unavailable: %s", cmd);
        case CR_FAILURE:
            return luaL_error(l, "console.eval failed: %s", cmd);
        default:
            return luaL_error(
                l, "console.eval unknown error (%d): %s", (int)result, cmd);
        }
    }
    return 0;
}

void LUA_CreateFunctions(lua_State *const l)
{
    lua_newtable(l);
    lua_pushcfunction(l, M_L_GetLaraItem);
    lua_setfield(l, -2, "get_item");
    lua_setglobal(l, "lara");

    lua_newtable(l);
    lua_pushcfunction(l, M_L_ConsoleLog);
    lua_setfield(l, -2, "log");
    lua_pushcfunction(l, M_L_ConsoleEval);
    lua_setfield(l, -2, "eval");
    lua_setglobal(l, "console");
}
