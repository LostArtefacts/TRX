// Lua event listener support
#pragma once

#include "./enum.h"

#include <stdint.h>

typedef enum {
    LUA_EVENT_ARG_NIL,
    LUA_EVENT_ARG_INT32,
    LUA_EVENT_ARG_BOOL,
    LUA_EVENT_ARG_NUMBER,
    LUA_EVENT_ARG_STRING,
} LUA_EVENT_ARG_TYPE;

typedef struct {
    LUA_EVENT_ARG_TYPE type;
    union {
        int32_t i32;
        bool b;
        double number;
        const char *str;
    } value;
} LUA_EVENT_ARG;

// Fire a Lua event. Answers whether a handler returned a true value, which is
// how a script takes over an event that carries a default; an event without
// one ignores the answer.
//
// Fire a Lua event of given type with arbitrary arguments
bool LUA_FireEventEx(
    LUA_EVENT_TYPE ev, const LUA_EVENT_ARG *args, int32_t arg_count);

// Fire a Lua event of given type with no arguments
bool LUA_FireEvent(LUA_EVENT_TYPE ev);

// Fire a Lua event of given type with int32 argument
bool LUA_FireEventInt32(LUA_EVENT_TYPE ev, int32_t arg);

// Fire a Lua event of given type with boolean argument
bool LUA_FireEventBool(LUA_EVENT_TYPE ev, bool arg);

// Spelled out rather than taken from lua.h, which game code that fires events
// does not build against.
typedef struct lua_State lua_State;

// What a listener with no dispatch key holds. A keyed fire reaches only the
// listeners that hold its key.
#define LUA_EVENT_NO_KEY (-1)

// How many event types a module of the public surface may declare on top of the
// engine's own.
#define LUA_EVENT_MAX_DECLARED 32

// How many event types there are, the engine's and the declared ones.
int32_t LUA_Events_GetTypeCount(void);

// Attaches the function at `fn_idx` as a listener, scoped to the script
// running, and returns the id it is detached by. A key claims its number for
// the script, as a flip effect listener does.
int32_t LUA_Events_Attach(
    lua_State *L, LUA_EVENT_TYPE ev, int fn_idx, int32_t key);

// Returns the event type a module of the public surface names `name` by,
// declaring it the first time, or -1 where there is no room for another.
LUA_EVENT_TYPE LUA_Events_Declare(const char *name);

// Detaches the listener with this id. False where none is attached.
bool LUA_Events_Detach(lua_State *L, int32_t id);
