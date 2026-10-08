#include <trx/game/lua/common.h>
#include <trx/game/lua/events.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/utils/args.h>
#include <trx/game/lua/utils/module.h>

#include <lauxlib.h>

#define M_MAX_FIRE_ARGS 4

// trxc.events.attach(event_type: integer, callback: function, key?: integer): integer
static int M_L_EventsAttach(lua_State *const L)
{
    const LUA_EVENT_TYPE ev = (LUA_EVENT_TYPE)LUA_CheckRange(
        L, 1, LUA_Events_GetTypeCount(), "unknown event type");
    luaL_checktype(L, 2, LUA_TFUNCTION);

    // A flip effect listener keys on the number it dispatches for, and its
    // attach is what claims the key: everything is validated by then, so a
    // failed attach claims nothing.
    int32_t key = LUA_EVENT_NO_KEY;
    if (ev == LUA_EVENT_FLIP_EFFECT) {
        key = luaL_checkinteger(L, 3);
    }

    lua_pushinteger(L, LUA_Events_Attach(L, ev, 2, key));
    return 1;
}

// trxc.events.declare(name: string): integer
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
    const LUA_EVENT_TYPE ev = LUA_Events_Declare(name);
    if (ev < 0) {
        return luaL_error(
            L, "no room to declare event '%s': the limit is %d", name,
            LUA_EVENT_MAX_DECLARED);
    }
    lua_pushinteger(L, ev);
    return 1;
}

// trxc.events.fire(event_type: integer, ...: boolean|number|string|nil): boolean
//
// For a module of the public surface that is itself an event source: the zones
// find their transitions in Lua and report them here, so a handler attaches,
// answers and detaches exactly as it does for an event the engine raises. trxc
// is off the globals before any script runs, so this stays the surface's own.
static int M_L_EventsFire(lua_State *const L)
{
    const LUA_EVENT_TYPE ev = (LUA_EVENT_TYPE)LUA_CheckRange(
        L, 1, LUA_Events_GetTypeCount(), "unknown event type");

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

// trxc.events.detach(id: integer): boolean
static int M_L_EventsDetach(lua_State *const L)
{
    lua_pushboolean(L, LUA_Events_Detach(L, luaL_checkinteger(L, 1)));
    return 1;
}

// trxc.events.is_level_script(): boolean
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

static void M_Create(lua_State *const L)
{
    LUA_RegisterModule(L, "events", m_Module);
}

REGISTER_LUA_CAPI(.create = M_Create)
