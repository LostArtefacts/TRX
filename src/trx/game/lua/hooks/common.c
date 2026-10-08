#include <trx/game/lua/hooks/common.h>

#include <trx/core/memory.h>
#include <trx/debug.h>
#include <trx/game/console.h>
#include <trx/game/lua/common.h>
#include <trx/game/lua/registry.h>

#include <lauxlib.h>
#include <string.h>
#include <uthash.h>

// Hooks are kept by hook and key, so this knows nothing of what a hook decides
// or how a script names it: a hook's handlers say that for it. A hook set by a
// level script ends with the level; one set by the game script or a module
// stays.

typedef struct {
    int64_t id;
    int32_t ref;
    bool level_scoped;
    UT_hash_handle hh;
} M_HOOK;

static const char *const m_Names[LUA_HOOK_NUMBER_OF] = {
#define X_HOOK(id, name) [id] = #name,
#include <trx/game/lua/hooks/hooks.def>
#undef X_HOOK
};

static LUA_HOOK_HANDLERS m_Handlers[LUA_HOOK_NUMBER_OF] = {};
static M_HOOK *m_Hooks = nullptr;
static lua_State *m_L = nullptr;

static int64_t M_GetID(const LUA_HOOK_TYPE hook, const int32_t key)
{
    return ((int64_t)hook << 32) | (uint32_t)key;
}

static M_HOOK *M_Find(const LUA_HOOK_TYPE hook, const int32_t key)
{
    const int64_t id = M_GetID(hook, key);
    M_HOOK *entry = nullptr;
    HASH_FIND(hh, m_Hooks, &id, sizeof(id), entry);
    return entry;
}

static void M_Clear(M_HOOK *const entry)
{
    const LUA_HOOK_TYPE hook = (LUA_HOOK_TYPE)(entry->id >> 32);
    if (m_Handlers[hook].on_clear != nullptr) {
        m_Handlers[hook].on_clear(hook, (int32_t)(uint32_t)entry->id);
    }
    if (m_L != nullptr) {
        luaL_unref(m_L, LUA_REGISTRYINDEX, entry->ref);
    }
    HASH_DEL(m_Hooks, entry);
    Memory_Free(entry);
}

static void M_Shutdown(void)
{
    M_HOOK *entry;
    M_HOOK *tmp;
    HASH_ITER(hh, m_Hooks, entry, tmp)
    {
        M_Clear(entry);
    }
    m_L = nullptr;
}

static void M_DropLevel(void)
{
    M_HOOK *entry;
    M_HOOK *tmp;
    HASH_ITER(hh, m_Hooks, entry, tmp)
    {
        if (entry->level_scoped) {
            M_Clear(entry);
        }
    }
}

static void M_Create(lua_State *const L)
{
    m_L = L;
}

LUA_HOOK_TYPE LUA_Hooks_GetByName(const char *const name)
{
    LUA_HOOK_TYPE hook = 0;
    while (hook < LUA_HOOK_NUMBER_OF && strcmp(m_Names[hook], name) != 0) {
        hook++;
    }
    return hook;
}

int32_t LUA_Hooks_ReadKey(
    lua_State *const L, const LUA_HOOK_TYPE hook, const int arg)
{
    ASSERT(hook >= 0 && hook < LUA_HOOK_NUMBER_OF);
    const LUA_HOOK_HANDLERS *const handlers = &m_Handlers[hook];
    return handlers->read_key != nullptr ? handlers->read_key(L, arg) : 0;
}

void LUA_Hooks_Set(
    lua_State *const L, const LUA_HOOK_TYPE hook, const int32_t key,
    const int fn_idx)
{
    ASSERT(hook >= 0 && hook < LUA_HOOK_NUMBER_OF);
    M_HOOK *const old = M_Find(hook, key);
    if (old != nullptr) {
        M_Clear(old);
    }
    if (lua_isnoneornil(L, fn_idx)) {
        return;
    }

    M_HOOK *const entry = Memory_Alloc(sizeof(M_HOOK));
    entry->id = M_GetID(hook, key);
    lua_pushvalue(L, fn_idx);
    entry->ref = luaL_ref(L, LUA_REGISTRYINDEX);
    entry->level_scoped = LUA_GetScriptContext() == LUA_CONTEXT_LEVEL;
    HASH_ADD(hh, m_Hooks, id, sizeof(entry->id), entry);
    if (m_Handlers[hook].on_set != nullptr) {
        m_Handlers[hook].on_set(hook, key);
    }
}

void LUA_Hooks_SetHandlers(
    const LUA_HOOK_TYPE hook, const LUA_HOOK_HANDLERS *const handlers)
{
    ASSERT(hook >= 0 && hook < LUA_HOOK_NUMBER_OF);
    m_Handlers[hook] = *handlers;
}

bool LUA_Hooks_IsSet(const LUA_HOOK_TYPE hook, const int32_t key)
{
    return M_Find(hook, key) != nullptr;
}

lua_State *LUA_Hooks_CallEx(
    const LUA_HOOK_TYPE hook, const int32_t key, const LUA_HOOK_ARG *const args,
    const int32_t arg_count, const int32_t result_count)
{
    ASSERT(hook >= 0 && hook < LUA_HOOK_NUMBER_OF);
    ASSERT(key == 0 || m_Handlers[hook].read_key != nullptr);
    const M_HOOK *const entry = M_Find(hook, key);
    if (m_L == nullptr || entry == nullptr) {
        return nullptr;
    }
    lua_rawgeti(m_L, LUA_REGISTRYINDEX, entry->ref);
    for (int32_t i = 0; i < arg_count; i++) {
        args[i].push(m_L, &args[i]);
    }
    // As with events, what the hook sets in turn is scoped where it is.
    const LUA_CONTEXT outer_context = LUA_GetScriptContext();
    LUA_SetScriptContext(
        entry->level_scoped ? LUA_CONTEXT_LEVEL : LUA_CONTEXT_GLOBAL);
    const bool ok = lua_pcall(m_L, arg_count, result_count, 0) == LUA_OK;
    LUA_SetScriptContext(outer_context);
    if (!ok) {
        Console_ShowError(
            "%s hook error: %s", m_Names[hook], lua_tostring(m_L, -1));
        lua_pop(m_L, 1);
        return nullptr;
    }
    return m_L;
}

bool LUA_Hooks_CallBoolEx(
    const LUA_HOOK_TYPE hook, const int32_t key, const bool fallback,
    const LUA_HOOK_ARG *const args, const int32_t arg_count)
{
    lua_State *const L = LUA_Hooks_CallEx(hook, key, args, arg_count, 1);
    if (L == nullptr) {
        return fallback;
    }
    const bool result = lua_isnil(L, -1) ? fallback : lua_toboolean(L, -1);
    lua_pop(L, 1);
    return result;
}

int32_t LUA_Hooks_CallIntEx(
    const LUA_HOOK_TYPE hook, const int32_t key, const int32_t fallback,
    const LUA_HOOK_ARG *const args, const int32_t arg_count)
{
    lua_State *const L = LUA_Hooks_CallEx(hook, key, args, arg_count, 1);
    if (L == nullptr) {
        return fallback;
    }
    int32_t result = fallback;
    if (lua_isinteger(L, -1)) {
        result = (int32_t)lua_tointeger(L, -1);
    } else if (!lua_isnil(L, -1)) {
        Console_ShowError("%s hook error: expected an integer", m_Names[hook]);
    }
    lua_pop(L, 1);
    return result;
}

REGISTER_LUA_CAPI(
        .create = M_Create, .shutdown = M_Shutdown, .drop_level = M_DropLevel)
