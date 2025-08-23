#include "game/lua/common.h"

#include "debug.h"
#include "game/lua/funcs.h"
#include "game/lua/structs.h"
#include "memory.h"

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

typedef struct {
    lua_State *state;
} M_PRIV;

static M_PRIV m_Priv = {};

void LUA_Init(void)
{
    M_PRIV *const p = &m_Priv;
    p->state = luaL_newstate();
    ASSERT(p->state != nullptr);
    luaL_openlibs(p->state);

    LUA_CreateStructs(p->state);
    LUA_CreateFunctions(p->state);
}

void LUA_Shutdown(void)
{
    M_PRIV *const p = &m_Priv;
    lua_close(p->state);
    p->state = nullptr;
}

LUA_RESULT Lua_Eval(const char *const code)
{
    M_PRIV *const p = &m_Priv;
    lua_State *l = p->state;
    LUA_RESULT result = { .code = LUA_OK, .message = nullptr };
    int status = luaL_loadstring(l, code);
    if (status != LUA_OK) {
        result.code = status;
        result.message = Memory_DupStr(lua_tostring(l, -1));
        lua_pop(l, 1);
        return result;
    }
    status = lua_pcall(l, 0, LUA_MULTRET, 0);
    if (status != LUA_OK) {
        result.code = status;
        result.message = Memory_DupStr(lua_tostring(l, -1));
        lua_pop(l, 1);
    }
    return result;
}

void Lua_FreeResult(LUA_RESULT *const result)
{
    if (result != nullptr) {
        Memory_FreePointer(&result->message);
    }
}
