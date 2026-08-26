#include <trx/core/log.h>
#include <trx/core/memory.h>
#include <trx/core/vector.h>
#include <trx/game/console/common.h>
#include <trx/game/items/actions.h>
#include <trx/game/lua/common.h>
#include <trx/game/lua/events.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/utils.h>

#include <lauxlib.h>
#include <lua.h>
#include <string.h>

#define M_NO_KEY (-1)
#define M_MAX_FIRE_ARGS 4
// How many event types a module of the public surface may declare on top of the
// engine's own. Reaching it raises rather than dropping the declaration, so a
// module that needs more says so at boot.
#define M_MAX_DECLARED 32

typedef struct {
    // What a script detaches by. Its own, and not its Lua ref: luaL_unref hands
    // a ref back out to the next attach, so a detached listener's ref can come
    // back as another's.
    int32_t id;
    int32_t ref;
    LUA_EVENT_TYPE type;
    // An optional dispatch key: a keyed fire reaches only the listeners
    // holding the same key. M_NO_KEY on both sides means unkeyed. Flip
    // effects key on the claimed effect number.
    int32_t key;
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
    // What the handler returned, as a yes or no. An event that carries a
    // default the script may take over reads it; the rest ignore it.
    bool answered;
} M_DISPATCH;

// A key that has been attached for an event at least once, per M_IsKeyClaimed.
typedef struct {
    LUA_EVENT_TYPE type;
    int32_t key;
    bool level_scoped;
} M_CLAIM;

static lua_State *m_L = nullptr;
static VECTOR *m_Listeners = nullptr;
static VECTOR *m_Claims = nullptr;
static int32_t m_NextId = 1;
static int32_t m_DispatchDepth = 0;
// Live listeners per type, so a fire for a type nobody watches returns without
// walking the vector. A listener counts until it stops being able to fire,
// which M_RemoveListener decides for both its branches.
static int32_t m_ListenerCount[LUA_EVENT_NUMBER_OF + M_MAX_DECLARED] = { 0 };

// The event types declared from Lua, which follow the engine's own. A
// declaration is the name of an event rather than any state of one, so it
// outlives a shutdown: the id a module took at load time still names the same
// event once the listeners have been cleared and remade.
static char *m_Declared[M_MAX_DECLARED] = { nullptr };
static int32_t m_DeclaredCount = 0;

static int32_t M_TypeCount(void)
{
    return LUA_EVENT_NUMBER_OF + m_DeclaredCount;
}

static bool M_FireEvent(
    LUA_EVENT_TYPE ev, int32_t key, const LUA_EVENT_ARG *args,
    int32_t arg_count);

// Removing outright would move the listeners out from under a dispatch in
// flight. While one is up, a removed listener is only marked, and the vector is
// compacted once the last one unwinds.
static void M_RemoveListener(lua_State *const L, const int32_t i)
{
    M_LISTENER *const lst = Vector_Get(m_Listeners, i);
    luaL_unref(L, LUA_REGISTRYINDEX, lst->ref);
    lst->ref = LUA_NOREF;
    m_ListenerCount[lst->type]--;
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
    for (int32_t i = 0; i < M_TypeCount(); i++) {
        m_ListenerCount[i] = 0;
    }
}

static void M_Shutdown(void)
{
    ItemAction_SetInterceptor(nullptr);
    M_ClearAllListeners(true);
    if (m_Claims != nullptr) {
        Vector_Free(m_Claims);
        m_Claims = nullptr;
    }
    m_L = nullptr;
}

// The state is gone by now, so a listener's ref cannot be given back to it. The
// names go here rather than in M_Shutdown, which a declaration outlives.
__attribute__((destructor)) static void M_AtExit(void)
{
    M_Shutdown();
    for (int32_t i = 0; i < m_DeclaredCount; i++) {
        Memory_FreePointer(&m_Declared[i]);
    }
    m_DeclaredCount = 0;
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
    M_DISPATCH *const dispatch = lua_touserdata(L, 1);
    if (lua_checkstack(L, dispatch->arg_count + 1) == 0) {
        return luaL_error(L, "no room for the handler and its arguments");
    }
    lua_rawgeti(L, LUA_REGISTRYINDEX, dispatch->ref);
    for (int32_t i = 0; i < dispatch->arg_count; i++) {
        M_PushArg(L, dispatch->args[i]);
    }
    lua_call(L, dispatch->arg_count, 1);
    dispatch->answered = lua_toboolean(L, -1);
    lua_pop(L, 1);
    return 0;
}

// A keyed listener claims its key as it attaches. Claims are not refcounted:
// a claim stays until its scope ends even if every listener for the key
// detaches first.
static void M_ClaimKey(const LUA_EVENT_TYPE ev, const int32_t key)
{
    const M_CLAIM claim = {
        .type = ev,
        .key = key,
        .level_scoped = LUA_GetScriptContext() == LUA_CONTEXT_LEVEL,
    };
    if (m_Claims == nullptr) {
        m_Claims = Vector_Create(sizeof(M_CLAIM));
    }
    for (int32_t i = 0; i < m_Claims->count; i++) {
        const M_CLAIM *const existing = Vector_Get(m_Claims, i);
        if (existing->type == claim.type && existing->key == claim.key
            && existing->level_scoped == claim.level_scoped) {
            return;
        }
    }
    Vector_Add(m_Claims, &claim);
}

// trxc.events.attach(event_type, callback, [key]) → id
static int M_L_EventsAttach(lua_State *const L)
{
    const LUA_EVENT_TYPE ev = (LUA_EVENT_TYPE)LUA_CheckRange(
        L, 1, M_TypeCount(), "unknown event type");
    luaL_checktype(L, 2, LUA_TFUNCTION);

    // A flip effect listener keys on the number it dispatches for, and its
    // attach is what claims the key: everything is validated by the time the
    // claim is recorded, so a failed attach claims nothing.
    int32_t key = M_NO_KEY;
    if (ev == LUA_EVENT_FLIP_EFFECT) {
        key = luaL_checkinteger(L, 3);
    }
    if (key != M_NO_KEY) {
        M_ClaimKey(ev, key);
    }

    lua_pushvalue(L, 2);
    const int32_t ref = luaL_ref(L, LUA_REGISTRYINDEX);
    if (m_Listeners == nullptr) {
        m_Listeners = Vector_Create(sizeof(M_LISTENER));
    }
    const M_LISTENER listener = {
        .id = m_NextId++,
        .ref = ref,
        .type = ev,
        .key = key,
        .level_scoped = LUA_GetScriptContext() == LUA_CONTEXT_LEVEL,
    };
    Vector_Add(m_Listeners, &listener);
    m_ListenerCount[ev]++;
    lua_pushinteger(L, listener.id);
    return 1;
}

// trxc.events.declare(name) -> event_type
//
// An event of a module's own. The engine's events are the LUA_EVENT_TYPE enum,
// which is where an event C raises has to be named; one a module of the public
// surface raises itself - the zones do - has nothing to do with C and is
// declared here instead. What comes back is an event type like any other, so
// attach, fire and detach take it and a level script's listeners are dropped
// with the level.
//
// Declaring the same name twice hands back the same type, so a module can ask
// for its own events without keeping track of whether it already has.
static int M_L_EventsDeclare(lua_State *const L)
{
    const char *const name = luaL_checkstring(L, 1);
    for (int32_t i = 0; i < m_DeclaredCount; i++) {
        if (strcmp(m_Declared[i], name) == 0) {
            lua_pushinteger(L, LUA_EVENT_NUMBER_OF + i);
            return 1;
        }
    }
    if (m_DeclaredCount >= M_MAX_DECLARED) {
        return luaL_error(
            L, "no room to declare event '%s': the limit is %d", name,
            M_MAX_DECLARED);
    }
    m_Declared[m_DeclaredCount] = Memory_DupStr(name);
    lua_pushinteger(L, LUA_EVENT_NUMBER_OF + m_DeclaredCount);
    m_DeclaredCount++;
    return 1;
}

// trxc.events.fire(event_type, ...) -> bool
//
// For a module of the public surface that is itself an event source: the zones
// find their transitions in Lua and report them here, so a handler attaches,
// answers and detaches exactly as it does for an event the engine raises. trxc
// is off the globals before any script runs, so this stays the surface's own.
static int M_L_EventsFire(lua_State *const L)
{
    const LUA_EVENT_TYPE ev = (LUA_EVENT_TYPE)LUA_CheckRange(
        L, 1, M_TypeCount(), "unknown event type");

    const int32_t count = lua_gettop(L) - 1;
    if (count > M_MAX_FIRE_ARGS) {
        return luaL_error(
            L, "an event takes at most %d arguments, got %d", M_MAX_FIRE_ARGS,
            count);
    }

    LUA_EVENT_ARG args[M_MAX_FIRE_ARGS];
    for (int32_t i = 0; i < count; i++) {
        const int arg = i + 2;
        switch (lua_type(L, arg)) {
        case LUA_TNIL:
            args[i] = (LUA_EVENT_ARG) { .type = LUA_EVENT_ARG_NIL };
            break;
        case LUA_TBOOLEAN:
            args[i] = (LUA_EVENT_ARG) {
                .type = LUA_EVENT_ARG_BOOL,
                .value = { .b = lua_toboolean(L, arg) },
            };
            break;
        case LUA_TNUMBER:
            if (lua_isinteger(L, arg)) {
                args[i] = (LUA_EVENT_ARG) {
                    .type = LUA_EVENT_ARG_INT32,
                    .value = { .i32 = (int32_t)lua_tointeger(L, arg) },
                };
            } else {
                args[i] = (LUA_EVENT_ARG) {
                    .type = LUA_EVENT_ARG_NUMBER,
                    .value = { .number = lua_tonumber(L, arg) },
                };
            }
            break;
        case LUA_TSTRING:
            // The argument stays on the stack for the whole dispatch, so what
            // this points at outlives the handlers reading it.
            args[i] = (LUA_EVENT_ARG) {
                .type = LUA_EVENT_ARG_STRING,
                .value = { .str = lua_tostring(L, arg) },
            };
            break;
        default:
            return luaL_argerror(
                L, arg, "an event carries no value of this type");
        }
    }

    lua_pushboolean(L, LUA_FireEventEx(ev, args, count));
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

// trxc.events.is_level_script() -> bool
static int M_L_EventsIsLevelScript(lua_State *const L)
{
    lua_pushboolean(L, LUA_GetScriptContext() == LUA_CONTEXT_LEVEL);
    return 1;
}

static const luaL_Reg m_Module[] = {
    { "attach", M_L_EventsAttach },
    { "declare", M_L_EventsDeclare },
    { "detach", M_L_EventsDetach },
    { "fire", M_L_EventsFire },
    { "is_level_script", M_L_EventsIsLevelScript },
    { nullptr, nullptr },
};

static bool M_IsKeyClaimed(const LUA_EVENT_TYPE ev, const int32_t key)
{
    if (m_Claims == nullptr) {
        return false;
    }
    for (int32_t i = 0; i < m_Claims->count; i++) {
        const M_CLAIM *const claim = Vector_Get(m_Claims, i);
        if (claim->type == ev && claim->key == key) {
            return true;
        }
    }
    return false;
}

// A claimed number fires to its listeners with the trigger's timer and the item
// that ran it.
static bool M_InterceptFlipEffect(
    const int32_t effect_num, const int32_t timer, const int16_t item_num)
{
    if (!M_IsKeyClaimed(LUA_EVENT_FLIP_EFFECT, effect_num)) {
        return false;
    }
    const LUA_EVENT_ARG args[] = {
        { .type = LUA_EVENT_ARG_INT32, .value = { .i32 = timer } },
        { .type = LUA_EVENT_ARG_INT32, .value = { .i32 = item_num } },
    };
    M_FireEvent(LUA_EVENT_FLIP_EFFECT, effect_num, args, 2);
    return true;
}

static void M_Create(lua_State *const L)
{
    m_L = L;

    LUA_RegisterModule(L, "events", m_Module);
    ItemAction_SetInterceptor(M_InterceptFlipEffect);
}

static bool M_FireEvent(
    const LUA_EVENT_TYPE ev, const int32_t key, const LUA_EVENT_ARG *const args,
    const int32_t arg_count)
{
    lua_State *const L = m_L;
    if (L == nullptr || m_Listeners == nullptr || m_ListenerCount[ev] == 0) {
        return false;
    }

    // Room for the protected call and its argument. Taken before the depth goes
    // up: from here to the matching decrement nothing may raise, or the
    // listeners would never be compacted again.
    if (lua_checkstack(L, 2) == 0) {
        Console_ShowError(
            "Lua stack exhausted; event %d not dispatched", (int32_t)ev);
        return false;
    }

    // A handler may attach while the event is in flight. Anything it adds lands
    // past the count taken here, so it waits for the next event.
    const int32_t count = m_Listeners->count;
    m_DispatchDepth++;

    const LUA_CONTEXT outer_context = LUA_GetScriptContext();
    bool answered = false;
    for (int32_t i = 0; i < count; i++) {
        // Re-read each time: an attach can move the vector.
        const M_LISTENER *const lst = Vector_Get(m_Listeners, i);
        if (lst->type != ev || lst->key != key || lst->dead) {
            continue;
        }
        M_DISPATCH dispatch = {
            .ref = lst->ref,
            .args = args,
            .arg_count = arg_count,
        };
        // A handler runs as the script that attached it, so what it does while
        // the event is in flight belongs where the handler does: a listener it
        // attaches in turn is scoped the same, and a module it requires goes
        // with the level rather than outliving it.
        LUA_SetScriptContext(
            lst->level_scoped ? LUA_CONTEXT_LEVEL : LUA_CONTEXT_GLOBAL);
        lua_pushcfunction(L, M_CallListener);
        lua_pushlightuserdata(L, &dispatch);
        if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
            Console_ShowError(
                "Lua event handler error: %s", lua_tostring(L, -1));
            lua_pop(L, 1);
        }
        LUA_SetScriptContext(outer_context);
        answered = answered || dispatch.answered;
    }

    m_DispatchDepth--;
    if (m_DispatchDepth == 0) {
        M_CompactListeners();
    }
    return answered;
}

void LUA_ClearLevelListeners(void)
{
    lua_State *const L = m_L;
    if (L == nullptr) {
        return;
    }

    if (m_Claims != nullptr) {
        for (int32_t i = 0; i < m_Claims->count;) {
            const M_CLAIM *const claim = Vector_Get(m_Claims, i);
            if (claim->level_scoped) {
                Vector_RemoveAt(m_Claims, i);
            } else {
                i++;
            }
        }
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

bool LUA_FireEventEx(
    const LUA_EVENT_TYPE ev, const LUA_EVENT_ARG *const args,
    const int32_t arg_count)
{
    return M_FireEvent(ev, M_NO_KEY, args, arg_count);
}

bool LUA_FireEvent(const LUA_EVENT_TYPE ev)
{
    return LUA_FireEventEx(ev, nullptr, 0);
}

bool LUA_FireEventInt32(const LUA_EVENT_TYPE ev, const int32_t arg)
{
    const LUA_EVENT_ARG args[] = {
        { .type = LUA_EVENT_ARG_INT32, .value = { .i32 = arg } },
    };
    return LUA_FireEventEx(ev, args, 1);
}

bool LUA_FireEventBool(const LUA_EVENT_TYPE ev, const bool arg)
{
    const LUA_EVENT_ARG args[] = {
        { .type = LUA_EVENT_ARG_BOOL, .value = { .b = arg } },
    };
    return LUA_FireEventEx(ev, args, 1);
}

REGISTER_LUA_CAPI(.create = M_Create, .shutdown = M_Shutdown)
