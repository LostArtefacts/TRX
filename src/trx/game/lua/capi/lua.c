// Evaluating Lua at runtime, over the same entry points the engine uses for
// the /lua command and for level scripts.

#include <trx/game/lua/common.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/utils.h>

#include <lauxlib.h>

// Nothing on success; a kind ("syntax" or "runtime") and a message otherwise.
static int M_PushResult(lua_State *const L, LUA_RESULT result)
{
    if (result.code == LUA_OK) {
        LUA_FreeResult(&result);
        return 0;
    }
    lua_pushstring(L, result.code == LUA_ERRSYNTAX ? "syntax" : "runtime");
    lua_pushstring(L, result.message != nullptr ? result.message : "");
    LUA_FreeResult(&result);
    return 2;
}

// trxc.lua.eval_expr(code) -> nil | (kind, message)
static int M_L_LuaEvalExpr(lua_State *const L)
{
    return M_PushResult(L, LUA_Eval(luaL_checkstring(L, 1)));
}

// trxc.lua.eval_file(path) -> nil | (kind, message)
static int M_L_LuaEvalFile(lua_State *const L)
{
    return M_PushResult(L, LUA_EvalFile(luaL_checkstring(L, 1)));
}

static const luaL_Reg m_Module[] = {
    { "eval_expr", M_L_LuaEvalExpr },
    { "eval_file", M_L_LuaEvalFile },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    LUA_RegisterModule(L, "lua", m_Module);
}

REGISTER_LUA_CAPI(.create = M_Create)
