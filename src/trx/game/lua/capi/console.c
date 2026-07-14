#include <trx/core/log.h>
#include <trx/game/console/common.h>
#include <trx/game/console/registry.h>
#include <trx/game/game_strings/entries.h>
#include <trx/game/lua/common.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/utils.h>

#include <lauxlib.h>

// trxc.console.log(level, msg)
static int M_L_ConsoleLog(lua_State *const L)
{
    LUA_LOG_CALL call;
    LUA_CheckLogCall(L, &call);
    Console_LogEx(call.level, call.src, call.line, call.func, "%s", call.msg);
    return 0;
}

// trxc.console.clear()
static int M_L_ConsoleClear(lua_State *const L)
{
    Console_Clear();
    return 0;
}

// trxc.console.eval(cmd, { verbose = bool })
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
    const char *err = "unknown error";
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

static const luaL_Reg m_Module[] = {
    { "log", M_L_ConsoleLog },
    { "eval", M_L_ConsoleEval },
    { "clear", M_L_ConsoleClear },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    LUA_RegisterModule(L, "console", m_Module);
}

REGISTER_LUA_CAPI(.create = M_Create)
