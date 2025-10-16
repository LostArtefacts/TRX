#include "log.h"

#include <lauxlib.h>
#include <lua.h>

// Helper: fetch Lua call site info and emit a log message
static void M_LogGeneric(lua_State *const L, const LOG_LEVEL level)
{
    const char *const msg = luaL_checkstring(L, 1);
    lua_Debug ar;
    const char *src = "?";
    const char *func = "?";
    int line = 0;
    if (lua_getstack(L, 1, &ar) && lua_getinfo(L, "nSl", &ar)) {
        src = ar.short_src;
        func = ar.name ? ar.name : "?";
        line = ar.currentline;
    }
    Log_Message(level, src, line, func, "%s", msg);
}

static int M_L_LogInfo(lua_State *const L)
{
    M_LogGeneric(L, LOG_LEVEL_INFO);
    return 0;
}

static int M_L_LogWarning(lua_State *const L)
{
    M_LogGeneric(L, LOG_LEVEL_WARNING);
    return 0;
}

static int M_L_LogError(lua_State *const L)
{
    M_LogGeneric(L, LOG_LEVEL_ERROR);
    return 0;
}

static int M_L_LogDebug(lua_State *const L)
{
    M_LogGeneric(L, LOG_LEVEL_DEBUG);
    return 0;
}

void LUA_CreateLog(lua_State *const L)
{
    lua_getglobal(L, "trx");
    lua_newtable(L);
    lua_pushcfunction(L, M_L_LogInfo);
    lua_setfield(L, -2, "info");
    lua_pushcfunction(L, M_L_LogWarning);
    lua_setfield(L, -2, "warn");
    lua_pushcfunction(L, M_L_LogWarning);
    lua_setfield(L, -2, "warning");
    lua_pushcfunction(L, M_L_LogError);
    lua_setfield(L, -2, "error");
    lua_pushcfunction(L, M_L_LogDebug);
    lua_setfield(L, -2, "debug");
    lua_setfield(L, -2, "log");
    lua_pop(L, 1);
}
