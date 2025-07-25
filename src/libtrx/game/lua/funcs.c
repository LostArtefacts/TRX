#include "game/lua/funcs.h"

#include "game/console/common.h"
#include "game/lara.h"
#include "strings.h"

#include <lauxlib.h>

static int M_L_GetLaraItem(lua_State *l);
static int M_L_ConsoleLog(lua_State *l);

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

void LUA_CreateFunctions(lua_State *const l)
{
    lua_newtable(l);
    lua_pushcfunction(l, M_L_GetLaraItem);
    lua_setfield(l, -2, "get_item");
    lua_setglobal(l, "lara");

    lua_newtable(l);
    lua_pushcfunction(l, M_L_ConsoleLog);
    lua_setfield(l, -2, "log");
    lua_setglobal(l, "console");
}
