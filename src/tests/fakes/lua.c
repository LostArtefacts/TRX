// The eval entry points, run against the state FakeLua_SetState hands over:
// what M_LuaLoadAndRun does in common.c, minus the engine around it.

#include <fakes/lua.h>

#include <trx/core/memory.h>
#include <trx/game/lua/common.h>

#include <lauxlib.h>

static lua_State *m_State = nullptr;

static LUA_RESULT M_Finish(lua_State *const L, int32_t status)
{
    if (status == LUA_OK) {
        status = lua_pcall(L, 0, 0, 0);
    }
    LUA_RESULT result = { .code = status, .message = nullptr };
    if (status != LUA_OK) {
        result.message = Memory_DupStr(lua_tostring(L, -1));
        lua_pop(L, 1);
    }
    return result;
}

void FakeLua_SetState(lua_State *const L)
{
    m_State = L;
}

LUA_RESULT LUA_Eval(const char *const code)
{
    return M_Finish(m_State, luaL_loadstring(m_State, code));
}

LUA_RESULT LUA_EvalFile(const char *const path)
{
    return M_Finish(m_State, luaL_loadfile(m_State, path));
}

void LUA_FreeResult(LUA_RESULT *const result)
{
    Memory_FreePointer(&result->message);
    result->code = LUA_OK;
}
