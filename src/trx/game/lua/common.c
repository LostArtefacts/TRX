#include <trx/game/lua/common.h>

#include <trx/core/filesystem.h>
#include <trx/core/log.h>
#include <trx/core/memory.h>
#include <trx/core/shell.h>
#include <trx/core/strings.h>
#include <trx/debug.h>
#include <trx/game/console/common.h>
#include <trx/game/game_flow/common.h>
#include <trx/game/lua/api.h>
#include <trx/game/lua/embedded_scripts.h>
#include <trx/game/lua/events.h>
#include <trx/game/lua/guard.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/sandbox.h>
#include <trx/game/lua/utils.h>
#include <trx/game/paths.h>

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <stdio.h>
#include <string.h>

// The pool beside the engine, and the root a script may not require: the
// engine's own modules are reached as the global trx table.
#define M_COMMON_ROOT "common"
#define M_ENGINE_ROOT "trx"

typedef struct {
    lua_State *state;
    LUA_CONTEXT context;
    // Whether a level's script run is outstanding. Level_Unload runs at the top
    // of every level load, the first one included, where there is nothing to
    // let go of and nobody to tell about it. A probe run counts as one, because
    // it leaves the same state behind as a level being played.
    bool level_script_live;
} M_PRIV;

static M_PRIV m_Priv = {
    .context = LUA_CONTEXT_GLOBAL,
};

// What a game's scripts have required so far, one table per context. They live
// in the registry rather than in globals, so a script can neither read them nor
// plant an entry in one. A level's table goes with the level, in
// LUA_DropLevelModules.
static const char m_RequiredKeys[LUA_CONTEXT_NUMBER_OF] = {};

// Marks a script that is running, so one requiring itself is told rather than
// left to recurse. It is a lightuserdata of an address the engine owns, which
// no script can produce and no script can return, so a returned value is never
// mistaken for the mark - `false` is a value a module may well return.
static const char m_RunningMark = 0;

// Text only: a binary chunk goes through a loader that validates nothing,
// which is why LUA_HardenGlobals takes load() and string.dump away.
static int M_LoadFile(lua_State *const L, const char *const path)
{
    return luaL_loadfilex(L, path, "t");
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

static RESULT M_RequireTRXModule(lua_State *const L, const char *name)
{
    lua_getglobal(L, "require");
    lua_pushstring(L, name);
    FAIL_IF(
        lua_pcall(L, 1, LUA_MULTRET, 0) != LUA_OK, "%s: %s", name,
        lua_tostring(L, -1));
    lua_settop(L, 0);
    return OK;
}

// Sealing audits the API too, and only engine scripts declare, so a failure
// here says TRX's own data is wrong.
static RESULT M_SealPublicAPI(lua_State *const L)
{
    FAIL_IF(
        !LUA_API_PushEntrypoint(L, "seal"),
        "the Lua API registry handed over no sealer");
    FAIL_IF(
        lua_pcall(L, 0, 0, 0) != LUA_OK, "the Lua API would not seal: %s",
        lua_tostring(L, -1));
    lua_pushnil(L);
    lua_setglobal(L, "trxc");
    return OK;
}

static RESULT M_LoadTRXModules(lua_State *const L)
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
        const RESULT result = M_RequireTRXModule(L, name);
        Memory_FreePointer(&name);
        MUST(result);
    }
    return OK;
}

// Run after M_SealPublicAPI, so a script reaches all of trx.* without naming
// the parts it wants, and api.define raises by then: it cannot extend the API
// it consumes.
static RESULT M_RunTRXRuntimeScripts(lua_State *const L)
{
    for (const LUA_EMBEDDED_SCRIPT *script = g_LUA_EmbeddedRuntimeScripts;
         script->path != nullptr; script++) {
        LOG_DEBUG("Running TRX script %s", script->path);
        const char *const chunk_name =
            String_FormatStatic("@trx/%s", script->path);
        FAIL_IF(
            luaL_loadbuffer(
                L, (const char *)script->data, script->size, chunk_name)
                    != LUA_OK
                || lua_pcall(L, 0, 0, 0) != LUA_OK,
            "%s: %s", script->path, lua_tostring(L, -1));
        lua_settop(L, 0);
    }
    return OK;
}

// One or more segments joined by '.', as Lua names any other module:
// "my_module", "my_group.my_module". The shape is what keeps a require inside
// the directory it names - no empty segment to start, end or double a
// separator with, so no "..", and no '/' at all to climb out with.
static bool M_IsScriptName(const char *const name)
{
    size_t segment_len = 0;
    for (const char *ch = name; *ch != '\0'; ch++) {
        if (*ch == '.') {
            if (segment_len == 0) {
                return false;
            }
            segment_len = 0;
            continue;
        }
        const bool ok = (*ch >= 'a' && *ch <= 'z') || (*ch >= 'A' && *ch <= 'Z')
            || (*ch >= '0' && *ch <= '9') || *ch == '_' || *ch == '-';
        if (!ok) {
            return false;
        }
        segment_len++;
    }
    return segment_len != 0;
}

// Path resolution ignores case, so two spellings of one name reach one file.
// Keying by what the script wrote would run that file once for each spelling.
static const char *M_PushLowerName(lua_State *const L, const char *const name)
{
    const size_t len = strlen(name);
    luaL_Buffer buffer;
    char *const out = luaL_buffinitsize(L, &buffer, len);
    for (size_t i = 0; i < len; i++) {
        out[i] = (name[i] >= 'A' && name[i] <= 'Z') ? name[i] + ('a' - 'A')
                                                    : name[i];
    }
    luaL_pushresultsize(&buffer, len);
    return lua_tostring(L, -1);
}

static void M_ClearRequired(lua_State *const L, const LUA_CONTEXT context)
{
    lua_newtable(L);
    lua_rawsetp(L, LUA_REGISTRYINDEX, &m_RequiredKeys[context]);
}

// Whether a name's first segment is `root`, which is spelled out rather than
// compared with strncmp alone so that "commonplace.foo" is not read as the
// pool's "place.foo".
static bool M_HasRoot(
    const char *const name, const size_t root_len, const char *const root)
{
    return root_len == strlen(root) && strncmp(name, root, root_len) == 0;
}

// Where a rooted name lands. Everything but the pool is a game's own modules
// directory, named as the game sits in games/, so a script says which game it
// is reaching into and the answer does not depend on which game is running.
// modules/ holds what a script requires; scripts/ holds what the engine runs,
// and no name reaches it.
static const char *M_ResolveScript(
    lua_State *const L, const char *const name, const size_t root_len,
    const char *const stem)
{
    // The pieces are spelled into Lua strings rather than buffers of our own:
    // the resolver hands back a pointer into its own storage, and this way
    // there is nothing to free on the way out through luaL_error.
    const int32_t base = lua_gettop(L);

    // The dots the script wrote are the directories the file sits in.
    const char *const path_stem = luaL_gsub(L, stem, ".", "/");

    const bool is_common = M_HasRoot(name, root_len, M_COMMON_ROOT);
    const GAME_DYNAMIC_PATH source = is_common
        ? GAME_DYNAMIC_PATH_COMMON_MODULE_FILE
        : GAME_DYNAMIC_PATH_GAME_MODULE_FILE;

    const char *mod = nullptr;
    if (!is_common) {
        lua_pushlstring(L, name, root_len);
        mod = lua_tostring(L, -1);
    }

    // A name is the file it spells out, or the init.lua of a directory of that
    // name. A module that grows into several files keeps the name its callers
    // already write.
    const char *path = nullptr;
    for (int32_t i = 0; i < 2 && path == nullptr; i++) {
        const char *const tail =
            lua_pushfstring(L, i == 0 ? "%s.lua" : "%s/init.lua", path_stem);
        const char *const rel =
            is_common ? tail : lua_pushfstring(L, "%s/modules/%s", mod, tail);
        path = GamePath_PeekResolve(source, rel);
    }

    lua_settop(L, base);
    return path;
}

// require(name): "tr3-la.acme" is games/tr3-la/modules/acme.lua, and
// "common.acme" is the pool beside the engine. Runs the script once and hands
// every later call what it returned.
static int M_L_Require(lua_State *const L)
{
    // Errors name the call site's own spelling, the lookup the lowered one.
    const char *const raw = luaL_checkstring(L, 1);
    if (!M_IsScriptName(raw)) {
        return luaL_error(L, "not a script name: %s", raw);
    }

    lua_settop(L, 1);
    const char *const name = M_PushLowerName(L, raw);

    const char *const sep = strchr(name, '.');
    if (sep == nullptr) {
        return luaL_error(
            L, "a script name carries the directory it lives in: <game>.%s",
            raw);
    }
    const size_t root_len = (size_t)(sep - name);
    if (M_HasRoot(name, root_len, M_ENGINE_ROOT)) {
        return luaL_error(
            L,
            "%s: " M_ENGINE_ROOT
            ".* is the engine's own, reached as a "
            "global rather than required",
            raw);
    }

    const LUA_CONTEXT context = LUA_GetScriptContext();
    lua_rawgetp(L, LUA_REGISTRYINDEX, &m_RequiredKeys[context]);
    lua_getfield(L, 3, name);

    // Whoever requires a name first owns when the module goes: one the game
    // script required stays for the run, one a level script required goes with
    // the level. A require from the other context is handed the module that is
    // loaded, so it neither runs a second copy nor takes the name over.
    for (LUA_CONTEXT other = 0;
         lua_isnil(L, 4) && other < LUA_CONTEXT_NUMBER_OF; other++) {
        if (other == context) {
            continue;
        }
        lua_pop(L, 1);
        lua_rawgetp(L, LUA_REGISTRYINDEX, &m_RequiredKeys[other]);
        lua_getfield(L, -1, name);
        lua_remove(L, -2);
    }

    // Read from what the lookup found, so a cycle that crosses contexts is
    // reported rather than recursed.
    if (lua_touserdata(L, 4) == (void *)&m_RunningMark) {
        return luaL_error(L, "script requires itself: %s", raw);
    }
    if (!lua_isnil(L, 4)) {
        return 1;
    }
    lua_pop(L, 1);

    const char *const path = M_ResolveScript(L, name, root_len, sep + 1);
    if (path == nullptr) {
        return luaL_error(L, "no such script: %s", raw);
    }
    if (luaL_loadfilex(L, path, "t") != LUA_OK) {
        return luaL_error(L, "%s: %s", raw, lua_tostring(L, -1));
    }

    // Marked as running before it runs, so two scripts that require each other
    // are told so rather than running each other until Lua is out of stack.
    lua_pushlightuserdata(L, (void *)&m_RunningMark);
    lua_setfield(L, 3, name);
    if (lua_pcall(L, 0, 1, 0) != LUA_OK) {
        // The mark goes with the run it stood for. Left behind, the next
        // require of this name would be told the script requires itself.
        lua_pushnil(L);
        lua_setfield(L, 3, name);
        return lua_error(L);
    }

    // A script that returns nothing is still required only once.
    if (lua_isnil(L, 4)) {
        lua_pop(L, 1);
        lua_pushboolean(L, 1);
    }
    lua_pushvalue(L, 4);
    lua_setfield(L, 3, name);
    return 1;
}

RESULT LUA_Init(void)
{
    lua_State *const L = luaL_newstate();
    ASSERT(L != nullptr);
    LUA_OpenSafeLibs(L);
    LUA_Guard_Install(L, LUA_GUARD_BUDGET_SEC);

    lua_newtable(L);
    lua_setglobal(L, "trxc");
    lua_newtable(L);
    lua_setglobal(L, "trx");

    LUA_Registry_CreateAll(L);

    M_PRIV *const p = &m_Priv;
    p->state = L;

    MUST(M_LoadTRXModules(L));
    MUST(M_SealPublicAPI(L));
    MUST(M_RunTRXRuntimeScripts(L));
    LUA_HardenGlobals(L);
    LUA_InstallModRequire(L);
    return OK;
}

void LUA_Shutdown(void)
{
    M_PRIV *const p = &m_Priv;
    LUA_Registry_ShutdownAll();
    p->level_script_live = false;
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

void LUA_InstallModRequire(lua_State *const L)
{
    for (LUA_CONTEXT context = 0; context < LUA_CONTEXT_NUMBER_OF; context++) {
        M_ClearRequired(L, context);
    }

    lua_pushcfunction(L, M_L_Require);
    lua_setglobal(L, "require");
}

void LUA_DropLevelModules(lua_State *const L)
{
    if (L == nullptr) {
        return;
    }
    M_ClearRequired(L, LUA_CONTEXT_LEVEL);
}

void LUA_RunGameScript(void)
{
    // An expansion with nothing of its own to set up runs the script of the
    // game it extends, so it ships a file only to replace one. What it wants
    // to keep from the game it extends it requires by name.
    const char *const path =
        GamePath_PeekResolve(GAME_DYNAMIC_PATH_GAME_SCRIPT_FILE, "_game.lua");
    if (path == nullptr) {
        return;
    }

    LOG_INFO("Loading game script: %s", path);
    LUA_RESULT res = LUA_EvalFile(path);
    if (res.code != LUA_OK) {
        Console_ShowError("Lua game script error: %s", res.message);
    }
    LUA_FreeResult(&res);
}

void LUA_DropLevelScript(void)
{
    M_PRIV *const p = &m_Priv;
    if (p->level_script_live) {
        p->level_script_live = false;

        // Before the listeners go, so a module holding state the level set up
        // hears about it while its own handlers still answer and can take them
        // down itself.
        LUA_FireEvent(LUA_EVENT_LEVEL_UNLOAD);
    }

    LUA_ClearLevelListeners();
    LUA_Config_ClearLevelWatchers();
    LUA_Rooms_ClearFlipGroups();
    LUA_DropLevelModules(p->state);
}

void LUA_RunLevelScript(const GF_LEVEL *const level)
{
    m_Priv.level_script_live = true;
    LUA_SetScriptContext(LUA_CONTEXT_LEVEL);

    if (level->script_path != nullptr) {
        LUA_RESULT res = LUA_EvalFile(level->script_path);
        if (res.code != LUA_OK) {
            Console_ShowError("Lua level script error: %s", res.message);
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
    // The level stays where it is, so the unload that would otherwise let go of
    // the last run is not coming.
    LUA_DropLevelScript();
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
