#include <trx/core/log.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/utils.h>

#include <lauxlib.h>
#include <lua.h>

// trxc.log.log(level, msg)
static int M_L_LogGeneric(lua_State *const L)
{
    LUA_LOG_CALL call;
    LUA_CheckLogCall(L, &call);
    Log_Message(call.level, call.src, call.line, call.func, "%s", call.msg);
    return 0;
}

static const luaL_Reg m_Module[] = {
    { "log", M_L_LogGeneric },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    LUA_RegisterModule(L, "log", m_Module);
}

REGISTER_LUA_CAPI(.create = M_Create)
