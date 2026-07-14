#include <trx/game/lua/common.h>

#include <trx/core/filesystem.h>
#include <trx/core/log.h>
#include <trx/core/memory.h>
#include <trx/core/shell.h>
#include <trx/core/strings.h>
#include <trx/debug.h>
#include <trx/game/game_flow/common.h>
#include <trx/game/lua/api.h>
#include <trx/game/lua/embedded_scripts.h>
#include <trx/game/lua/events.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/sandbox.h>
#include <trx/game/lua/utils.h>

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    lua_State *state;
    LUA_CONTEXT context;
} M_PRIV;

static M_PRIV m_Priv = {
    .context = LUA_CONTEXT_GLOBAL,
};

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
    // This is the state's main stack, and it outlives the call.
    const int32_t base = lua_gettop(L);
    int status = loader(L, src);
    if (status != LUA_OK) {
        result.code = status;
        result.message = Memory_DupStr(lua_tostring(L, -1));
        lua_settop(L, base);
        return result;
    }
    status = lua_pcall(L, 0, LUA_MULTRET, 0);
    if (status != LUA_OK) {
        result.code = status;
        result.message = Memory_DupStr(lua_tostring(L, -1));
    }
    lua_settop(L, base);
    return result;
}

// Loader closure for embedded TRX modules, invoked via package.preload.
static int M_TRXEmbeddedModuleLoader(lua_State *const L)
{
    const uint8_t *const data = lua_touserdata(L, lua_upvalueindex(1));
    const size_t size = (size_t)lua_tointeger(L, lua_upvalueindex(2));
    const char *const chunk_name = lua_tostring(L, lua_upvalueindex(3));
    // require() calls a preload loader with the module name and ":preload:".
    // Those are still on the stack, and they are not the module.
    const int32_t base = lua_gettop(L);
    int status = luaL_loadbuffer(L, (const char *)data, size, chunk_name);
    if (status != LUA_OK) {
        lua_error(L);
    }
    status = lua_pcall(L, 0, LUA_MULTRET, 0);
    if (status != LUA_OK) {
        lua_error(L);
    }
    return lua_gettop(L) - base;
}

// The module name a script is required under: "objects/door.lua" becomes
// "trx.objects.door". Caller frees the result.
static char *M_DeriveTRXModuleName(const char *const path)
{
    char *stem = Memory_DupStr(path);
    const size_t stem_len = strlen(stem);
    if (stem_len > 4 && strcmp(stem + stem_len - 4, ".lua") == 0) {
        stem[stem_len - 4] = '\0';
    }
    for (char *c = stem; *c != '\0'; c++) {
        if (*c == '/') {
            *c = '.';
        }
    }
    char *const name = String_Format("trx.%s", stem);
    Memory_FreePointer(&stem);
    return name;
}

static void M_RegisterTRXPreloadEmbedded(
    lua_State *const L, const uint8_t *const data, const size_t size,
    const char *const chunk_name, const char *const name)
{
    lua_getglobal(L, "package");
    lua_getfield(L, -1, "preload");
    lua_pushlightuserdata(L, (void *)data);
    lua_pushinteger(L, (lua_Integer)size);
    lua_pushstring(L, chunk_name);
    lua_pushcclosure(L, M_TRXEmbeddedModuleLoader, 3);
    lua_setfield(L, -2, name);
    lua_pop(L, 2);
}

// Fatal for the reason a failure to seal is, in M_SealPublicAPI.
static void M_RequireTRXModule(lua_State *const L, const char *name)
{
    lua_getglobal(L, "require");
    lua_pushstring(L, name);
    if (lua_pcall(L, 1, LUA_MULTRET, 0) != LUA_OK) {
        Shell_ExitSystemFmt("failed to load %s: %s", name, lua_tostring(L, -1));
    }
    lua_settop(L, 0);
}

static void M_SealPublicAPI(lua_State *const L)
{
    // Sealing audits the API too, and only engine scripts declare - so a
    // failure here means our own data is wrong. Fatal, like any other bad
    // engine data.
    if (!LUA_API_PushEntrypoint(L, "seal")) {
        Shell_ExitSystem("the Lua API registry handed over no sealer");
    }
    if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
        Shell_ExitSystemFmt(
            "failed to seal the Lua API: %s", lua_tostring(L, -1));
    }
    lua_pushnil(L);
    lua_setglobal(L, "trxc");
}

static void M_LoadTRXModules(lua_State *const L)
{
    // Register every module's preload entry before requiring any of them.
    // Doing both in one pass would mean a module could only ever require one
    // that happens to appear earlier in the list, which silently couples the
    // Lua load order to the ordering of the source list in meson.build.
    for (const LUA_EMBEDDED_SCRIPT *script = g_LUA_EmbeddedModules;
         script->path != nullptr; script++) {
        char *name = M_DeriveTRXModuleName(script->path);
        const char *const chunk_name =
            String_FormatStatic(LUA_API_CHUNK_PREFIX "%s", script->path);
        M_RegisterTRXPreloadEmbedded(
            L, script->data, script->size, chunk_name, name);
        Memory_FreePointer(&name);
    }

    for (const LUA_EMBEDDED_SCRIPT *script = g_LUA_EmbeddedModules;
         script->path != nullptr; script++) {
        LOG_DEBUG("Loading TRX module %s", script->path);
        char *name = M_DeriveTRXModuleName(script->path);
        M_RequireTRXModule(L, name);
        Memory_FreePointer(&name);
    }
}

// Run after M_SealPublicAPI, so a script reaches all of trx.* without naming
// the parts it wants, and api.define raises by then: it cannot extend the API
// it consumes.
static void M_RunTRXRuntimeScripts(lua_State *const L)
{
    for (const LUA_EMBEDDED_SCRIPT *script = g_LUA_EmbeddedRuntimeScripts;
         script->path != nullptr; script++) {
        LOG_DEBUG("Running TRX script %s", script->path);
        const char *const chunk_name =
            String_FormatStatic("@trx/%s", script->path);
        if (luaL_loadbuffer(
                L, (const char *)script->data, script->size, chunk_name)
                != LUA_OK
            || lua_pcall(L, 0, 0, 0) != LUA_OK) {
            Shell_ExitSystemFmt(
                "failed to run %s: %s", script->path, lua_tostring(L, -1));
        }
        lua_settop(L, 0);
    }
}

void LUA_Init(void)
{
    lua_State *const L = luaL_newstate();
    ASSERT(L != nullptr);
    LUA_OpenSafeLibs(L);

    lua_newtable(L);
    lua_setglobal(L, "trxc");
    lua_newtable(L);
    lua_setglobal(L, "trx");

    LUA_Registry_CreateAll(L);

    M_PRIV *const p = &m_Priv;
    p->state = L;

    M_LoadTRXModules(L);
    M_SealPublicAPI(L);
    M_RunTRXRuntimeScripts(L);
    LUA_HardenGlobals(L);
}

void LUA_Shutdown(void)
{
    M_PRIV *const p = &m_Priv;
    LUA_Registry_ShutdownAll();
    if (p->state != nullptr) {
        lua_close(p->state);
        p->state = nullptr;
    }
}

LUA_CONTEXT LUA_GetScriptContext(void)
{
    M_PRIV *const p = &m_Priv;
    return p->context;
}

void LUA_SetScriptContext(const LUA_CONTEXT context)
{
    M_PRIV *const p = &m_Priv;
    p->context = context;
}

LUA_RESULT LUA_Eval(const char *const code)
{
    M_PRIV *const p = &m_Priv;
    return M_LuaLoadAndRun(p->state, luaL_loadstring, code);
}

LUA_RESULT LUA_EvalFile(const char *const path)
{
    M_PRIV *const p = &m_Priv;
    return M_LuaLoadAndRun(p->state, M_LoadFile, path);
}

void LUA_RunLevelScript(const GF_LEVEL *const level)
{
    LUA_ClearLevelListeners();
    LUA_SetScriptContext(LUA_CONTEXT_LEVEL);

    if (level->script_path != nullptr) {
        LUA_RESULT res = LUA_EvalFile(level->script_path);
        if (res.code != LUA_OK) {
            LOG_ERROR("Lua level script error: %s", res.message);
        }
        LUA_FreeResult(&res);
    }

    LUA_SetScriptContext(LUA_CONTEXT_GLOBAL);
}

void LUA_ReloadLevelScript(void)
{
    const GF_LEVEL *const level = GF_GetCurrentLevel();
    if (level == nullptr) {
        return;
    }
    LUA_RunLevelScript(level);
}

void LUA_FreeResult(LUA_RESULT *const result)
{
    if (result != nullptr) {
        Memory_FreePointer(&result->message);
    }
}

void LUA_DumpAPI(void)
{
    lua_State *const L = m_Priv.state;
    if (L == nullptr) {
        LOG_ERROR("--dump-lua-api: Lua is not initialised");
        return;
    }
    // Sealing has already run by now, so the dumper is no longer on trx.api.
    if (!LUA_API_PushEntrypoint(L, "to_json")) {
        LOG_ERROR("--dump-lua-api: the Lua API registry handed over no dumper");
        return;
    }
    if (lua_pcall(L, 0, 1, 0) != LUA_OK) {
        LOG_ERROR("--dump-lua-api failed: %s", lua_tostring(L, -1));
        lua_pop(L, 1);
        return;
    }
    const char *const json = lua_tostring(L, -1);
    if (json != nullptr) {
        puts(json);
    }
    lua_pop(L, 1);
}
