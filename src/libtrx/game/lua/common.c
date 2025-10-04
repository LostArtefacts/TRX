#include "game/lua/common.h"

#include "debug.h"
#include "filesystem.h"
#include "log.h"
#include "memory.h"

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

typedef struct {
    lua_State *state;
    LUA_CONTEXT context;
} M_PRIV;

static M_PRIV m_Priv = {
    .context = LUA_CONTEXT_GLOBAL,
};

// Initialize internal APIs
extern void LUA_CreateConsole(lua_State *L);
extern void LUA_CreateEvents(lua_State *L);
extern void LUA_CreateItems(lua_State *L);
extern void LUA_CreateLara(lua_State *L);
extern void LUA_CreateLog(lua_State *L);
extern void LUA_CreateMusic(lua_State *L);
extern void LUA_CreateSound(lua_State *L);
extern void LUA_CreateConfig(lua_State *L);

// Shared loader+pcall helper for Eval/EvalFile to capture errors with source
static LUA_RESULT M_LuaLoadAndRun(
    lua_State *const L, int (*const loader)(lua_State *, const char *),
    const char *const src)
{
    LUA_RESULT result = { .code = LUA_OK, .message = nullptr };
    int status = loader(L, src);
    if (status != LUA_OK) {
        result.code = status;
        result.message = Memory_DupStr(lua_tostring(L, -1));
        lua_pop(L, 1);
        return result;
    }
    status = lua_pcall(L, 0, LUA_MULTRET, 0);
    if (status != LUA_OK) {
        result.code = status;
        result.message = Memory_DupStr(lua_tostring(L, -1));
        lua_pop(L, 1);
    }
    return result;
}

void LUA_Init(void)
{
    lua_State *const L = luaL_newstate();
    ASSERT(L != nullptr);
    luaL_openlibs(L);

    lua_newtable(L);
    lua_setglobal(L, "TRX");

    // Initialize internal modules
    LUA_CreateConsole(L);
    LUA_CreateEvents(L);
    LUA_CreateItems(L);
    LUA_CreateLara(L);
    LUA_CreateLog(L);
    LUA_CreateMusic(L);
    LUA_CreateSound(L);
    LUA_CreateConfig(L);

    M_PRIV *const p = &m_Priv;
    p->state = L;
}

void LUA_Shutdown(void)
{
    M_PRIV *const p = &m_Priv;
    lua_close(p->state);
    p->state = nullptr;
}

LUA_CONTEXT Lua_GetScriptContext(void)
{
    M_PRIV *const p = &m_Priv;
    return p->context;
}

void Lua_SetScriptContext(const LUA_CONTEXT context)
{
    M_PRIV *const p = &m_Priv;
    p->context = context;
}

LUA_RESULT Lua_Eval(const char *const code)
{
    M_PRIV *const p = &m_Priv;
    return M_LuaLoadAndRun(p->state, luaL_loadstring, code);
}

LUA_RESULT Lua_EvalFile(const char *const path)
{
    M_PRIV *const p = &m_Priv;
    return M_LuaLoadAndRun(p->state, luaL_loadfile, path);
}

void Lua_FreeResult(LUA_RESULT *const result)
{
    if (result != nullptr) {
        Memory_FreePointer(&result->message);
    }
}
