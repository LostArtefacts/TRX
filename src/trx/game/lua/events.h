// Lua event listener support
#pragma once

#include <stdint.h>

// Event types for Lua listeners. Named in ENUM_MAP (see trx/game/enum.c), which
// is what the hooks in src/lua/api/events.lua reflect.
typedef enum {
    LUA_EVENT_GAME_START,
    LUA_EVENT_TITLE_START,
    LUA_EVENT_PICKUP,
    LUA_EVENT_BEFORE_CONTROL,
    LUA_EVENT_AFTER_CONTROL,
    LUA_EVENT_FLIP_EFFECT,
    LUA_EVENT_ROOM_CHANGE,
    LUA_EVENT_TRIGGER,
    LUA_EVENT_SHOW,
    LUA_EVENT_HIDE,
    LUA_EVENT_FINISH,
    LUA_EVENT_ENTER_SIM,
    LUA_EVENT_LEAVE_SIM,
    LUA_EVENT_ACTIVATE,
    LUA_EVENT_DEACTIVATE,
    LUA_EVENT_DESTROY,
    LUA_EVENT_ENTER_WORLD,
    LUA_EVENT_LEAVE_WORLD,
    LUA_EVENT_HIT,
    LUA_EVENT_KILL,
    LUA_EVENT_FLYBY_END,
    LUA_EVENT_CUTSCENE_TRIGGER,
    LUA_EVENT_CUTSCENE_START,
    LUA_EVENT_CUTSCENE_END,
    LUA_EVENT_LEVEL_UNLOAD,
    LUA_EVENT_UI_DRAW,
    LUA_EVENT_NUMBER_OF,
} LUA_EVENT_TYPE;

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

// Clear all listeners declared during the current level script
void LUA_ClearLevelListeners(void);

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
