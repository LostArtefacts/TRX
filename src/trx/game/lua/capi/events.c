#include <trx/core/log.h>
#include <trx/core/vector.h>
#include <trx/game/lua/common.h>
#include <trx/game/lua/events.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/utils.h>

#include <lauxlib.h>
#include <lua.h>

typedef struct {
    // What a script detaches by. Its own, and not its Lua ref: luaL_unref hands
    // a ref back out to the next attach, so a detached listener's ref can come
    // back as another's.
    int32_t id;
    int32_t ref;
    LUA_EVENT_TYPE type;
    bool level_scoped;
    bool dead;
} M_LISTENER;

// One listener's call. Setting it up allocates - a string argument is interned,
// and the stack may have to grow - so it can raise just as the call itself can.
// Run under lua_pcall, an error comes back to LUA_FireEventEx instead of
// unwinding past the dispatch depth it holds.
typedef struct {
    int32_t ref;
    const LUA_EVENT_ARG *args;
    int32_t arg_count;
} M_DISPATCH;

static lua_State *m_L = nullptr;
static VECTOR *m_Listeners = nullptr;
static int32_t m_NextId = 1;
static int32_t m_DispatchDepth = 0;

// Removing outright would move the listeners out from under a dispatch in
// flight. While one is up, a removed listener is only marked, and the vector is
// compacted once the last one unwinds.
static void M_RemoveListener(lua_State *const L, const int32_t i)
{
    M_LISTENER *const lst = Vector_Get(m_Listeners, i);
    luaL_unref(L, LUA_REGISTRYINDEX, lst->ref);
    lst->ref = LUA_NOREF;
    if (m_DispatchDepth > 0) {
        lst->dead = true;
    } else {
        Vector_RemoveAt(m_Listeners, i);
    }
}

static void M_CompactListeners(void)
{
    for (int32_t i = 0; i < m_Listeners->count;) {
        const M_LISTENER *const lst = Vector_Get(m_Listeners, i);
        if (lst->dead) {
            Vector_RemoveAt(m_Listeners, i);
        } else {
            i++;
        }
    }
}

static void M_ClearAllListeners(const bool unref_from_lua)
{
    if (m_Listeners == nullptr) {
        return;
    }

    if (unref_from_lua && m_L != nullptr) {
        for (int32_t i = 0; i < m_Listeners->count; i++) {
            const M_LISTENER *const lst = Vector_Get(m_Listeners, i);
            luaL_unref(m_L, LUA_REGISTRYINDEX, lst->ref);
        }
    }

    Vector_Free(m_Listeners);
    m_Listeners = nullptr;
}

static void M_Shutdown(void)
{
    M_ClearAllListeners(true);
    m_L = nullptr;
}

// The state is gone by now, so a listener's ref cannot be given back to it.
__attribute__((destructor)) static void M_AtExit(void)
{
    M_Shutdown();
}

static void M_PushArg(lua_State *const L, const LUA_EVENT_ARG arg)
{
    switch (arg.type) {
    case LUA_EVENT_ARG_NIL:
        lua_pushnil(L);
        break;
    case LUA_EVENT_ARG_INT32:
        lua_pushinteger(L, arg.value.i32);
        break;
    case LUA_EVENT_ARG_BOOL:
        lua_pushboolean(L, arg.value.b);
        break;
    case LUA_EVENT_ARG_NUMBER:
        lua_pushnumber(L, arg.value.number);
        break;
    case LUA_EVENT_ARG_STRING:
        if (arg.value.str != nullptr) {
            lua_pushstring(L, arg.value.str);
        } else {
            lua_pushnil(L);
        }
        break;
    }
}

static int M_CallListener(lua_State *const L)
{
    const M_DISPATCH *const dispatch = lua_touserdata(L, 1);
    if (lua_checkstack(L, dispatch->arg_count + 1) == 0) {
        return luaL_error(L, "no room for the handler and its arguments");
    }
    lua_rawgeti(L, LUA_REGISTRYINDEX, dispatch->ref);
    for (int32_t i = 0; i < dispatch->arg_count; i++) {
        M_PushArg(L, dispatch->args[i]);
    }
    lua_call(L, dispatch->arg_count, 0);
    return 0;
}

// trxc.events.attach(event_type, callback) → id
static int M_L_EventsAttach(lua_State *const L)
{
    const LUA_EVENT_TYPE ev = (LUA_EVENT_TYPE)LUA_CheckRange(
        L, 1, LUA_EVENT_NUMBER_OF, "unknown event type");
    luaL_checktype(L, 2, LUA_TFUNCTION);
    lua_pushvalue(L, 2);
    const int32_t ref = luaL_ref(L, LUA_REGISTRYINDEX);
    if (m_Listeners == nullptr) {
        m_Listeners = Vector_Create(sizeof(M_LISTENER));
    }
    const M_LISTENER listener = {
        .id = m_NextId++,
        .ref = ref,
        .type = ev,
        .level_scoped = LUA_GetScriptContext() == LUA_CONTEXT_LEVEL,
    };
    Vector_Add(m_Listeners, &listener);
    lua_pushinteger(L, listener.id);
    return 1;
}

// trxc.events.detach(id) -> bool
static int M_L_EventsDetach(lua_State *const L)
{
    const int32_t id = luaL_checkinteger(L, 1);
    if (m_Listeners == nullptr) {
        lua_pushboolean(L, false);
        return 1;
    }
    for (int32_t i = 0; i < m_Listeners->count; i++) {
        const M_LISTENER *const lst = Vector_Get(m_Listeners, i);
        if (lst->id == id && !lst->dead) {
            M_RemoveListener(L, i);
            lua_pushboolean(L, true);
            return 1;
        }
    }
    lua_pushboolean(L, false);
    return 1;
}

static const luaL_Reg m_Module[] = {
    { "attach", M_L_EventsAttach },
    { "detach", M_L_EventsDetach },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    m_L = L;

    LUA_RegisterModule(L, "events", m_Module);
}

void LUA_ClearLevelListeners(void)
{
    lua_State *const L = m_L;
    if (L == nullptr) {
        return;
    }
    if (m_Listeners == nullptr) {
        return;
    }
    for (int32_t i = 0; i < m_Listeners->count;) {
        const M_LISTENER *const lst = Vector_Get(m_Listeners, i);
        if (!lst->level_scoped || lst->dead) {
            i++;
            continue;
        }
        M_RemoveListener(L, i);
        if (m_DispatchDepth > 0) {
            i++;
        }
    }
}

void LUA_FireEventEx(
    const LUA_EVENT_TYPE ev, const LUA_EVENT_ARG *const args,
    const int32_t arg_count)
{
    lua_State *const L = m_L;
    if (L == nullptr || m_Listeners == nullptr) {
        return;
    }

    // Room for the protected call and its argument. Taken before the depth goes
    // up: from here to the matching decrement nothing may raise, or the
    // listeners would never be compacted again.
    if (lua_checkstack(L, 2) == 0) {
        LOG_ERROR("Lua stack exhausted; event %d not dispatched", (int32_t)ev);
        return;
    }

    // A handler may attach while the event is in flight. Anything it adds lands
    // past the count taken here, so it waits for the next event.
    const int32_t count = m_Listeners->count;
    m_DispatchDepth++;

    for (int32_t i = 0; i < count; i++) {
        // Re-read each time: an attach can move the vector.
        const M_LISTENER *const lst = Vector_Get(m_Listeners, i);
        if (lst->type != ev || lst->dead) {
            continue;
        }
        const M_DISPATCH dispatch = {
            .ref = lst->ref,
            .args = args,
            .arg_count = arg_count,
        };
        lua_pushcfunction(L, M_CallListener);
        lua_pushlightuserdata(L, (void *)&dispatch);
        if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
            LOG_ERROR("Lua event handler error: %s", lua_tostring(L, -1));
            lua_pop(L, 1);
        }
    }

    m_DispatchDepth--;
    if (m_DispatchDepth == 0) {
        M_CompactListeners();
    }
}

void LUA_FireEvent(const LUA_EVENT_TYPE ev)
{
    LUA_FireEventEx(ev, nullptr, 0);
}

void LUA_FireEventInt32(const LUA_EVENT_TYPE ev, const int32_t arg)
{
    const LUA_EVENT_ARG args[] = {
        { .type = LUA_EVENT_ARG_INT32, .value = { .i32 = arg } },
    };
    LUA_FireEventEx(ev, args, 1);
}

REGISTER_LUA_CAPI(.create = M_Create, .shutdown = M_Shutdown)
