#include "game/lua/common.h"

#include "debug.h"
#include "filesystem.h"
#include "log.h"
#include "memory.h"
#include "strings.h"

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <string.h>

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

static int M_LoadFile(lua_State *const L, const char *const path)
{
    return luaL_loadfile(L, path);
}

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

// Loader closure for TRX modules, invoked via package.preload.
static int M_TRXModuleLoader(lua_State *const L)
{
    const char *const path = lua_tostring(L, lua_upvalueindex(1));
    int status = luaL_loadfile(L, path);
    if (status != LUA_OK) {
        lua_error(L);
    }
    status = lua_pcall(L, 0, LUA_MULTRET, 0);
    if (status != LUA_OK) {
        lua_error(L);
    }
    // Return all values pushed by the chunk.
    return lua_gettop(L);
}

static void M_LoadTRXCModule(lua_State *const L, void (*loader)(lua_State *))
{
    LOG_INFO("Loading TRXC module %p", loader);
    loader(L);
}

static char *M_DeriveTRXModuleName(const char *path)
{
    char *raw = Memory_DupStr(path);
    size_t raw_len = strlen(raw);

    // Drop ".lua"
    if (raw_len > 4 && strcmp(raw + raw_len - 4, ".lua") == 0) {
        raw[raw_len - 4] = '\0';
    }

    // Convert '/' → '.'
    for (char *c = raw; *c; ++c) {
        if (*c == '/') {
            *c = '.';
        }
    }

    // Prefix "trx."
    const char *modprefix = "trx.";
    size_t prefix_len = strlen(modprefix);
    raw_len = strlen(raw);
    char *name = Memory_Alloc(prefix_len + raw_len + 1);
    memcpy(name, modprefix, prefix_len);
    memcpy(name + prefix_len, raw, raw_len + 1);

    Memory_FreePointer(&raw);
    return name;
}

static void M_RegisterTRXPreload(
    lua_State *const L, const char *path, const char *name)
{
    lua_getglobal(L, "package");
    lua_getfield(L, -1, "preload");
    lua_pushstring(L, path);
    lua_pushcclosure(L, M_TRXModuleLoader, 1);
    lua_setfield(L, -2, name);
    lua_pop(L, 2);
}

static void M_RequireTRXModule(lua_State *const L, const char *name)
{
    lua_getglobal(L, "require");
    lua_pushstring(L, name);
    if (lua_pcall(L, 1, LUA_MULTRET, 0) != LUA_OK) {
        LOG_ERROR("Failed to require module %s: %s", name, lua_tostring(L, -1));
        lua_pop(L, 1);
    }
    lua_settop(L, 0);
}

// Register a TRX Lua script as a package.preload module and require it.
static void M_LoadTRXModule(lua_State *const L, const char *const path)
{
    LOG_DEBUG("Loading TRX module %s", path);
    char *full_path =
        File_GetFullPath(String_FormatStatic("scripting/trx/%s", path));
    char *name = M_DeriveTRXModuleName(path);
    M_RegisterTRXPreload(L, full_path, name);
    M_RequireTRXModule(L, name);
    Memory_FreePointer(&full_path);
    Memory_FreePointer(&name);
}

void LUA_Init(void)
{
    lua_State *const L = luaL_newstate();
    ASSERT(L != nullptr);
    luaL_openlibs(L);

    lua_newtable(L);
    lua_setglobal(L, "trxc");
    lua_newtable(L);
    lua_setglobal(L, "trx");

    // Initialize internal modules
    M_LoadTRXCModule(L, LUA_CreateConsole);
    M_LoadTRXCModule(L, LUA_CreateEvents);
    M_LoadTRXCModule(L, LUA_CreateItems);
    M_LoadTRXCModule(L, LUA_CreateLara);
    M_LoadTRXCModule(L, LUA_CreateLog);
    M_LoadTRXCModule(L, LUA_CreateMusic);
    M_LoadTRXCModule(L, LUA_CreateSound);
    M_LoadTRXCModule(L, LUA_CreateConfig);

    M_PRIV *const p = &m_Priv;
    p->state = L;

    M_LoadTRXModule(L, "items.lua");
    M_LoadTRXModule(L, "config.lua");
    M_LoadTRXModule(L, "console.lua");
    M_LoadTRXModule(L, "events.lua");
    M_LoadTRXModule(L, "lara.lua");
    M_LoadTRXModule(L, "log.lua");
    M_LoadTRXModule(L, "sound.lua");
    M_LoadTRXModule(L, "music.lua");
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
    char *real_path = File_GetFullPath(path);
    const LUA_RESULT result = M_LuaLoadAndRun(p->state, M_LoadFile, real_path);
    Memory_FreePointer(&real_path);
    return result;
}

void Lua_FreeResult(LUA_RESULT *const result)
{
    if (result != nullptr) {
        Memory_FreePointer(&result->message);
    }
}
