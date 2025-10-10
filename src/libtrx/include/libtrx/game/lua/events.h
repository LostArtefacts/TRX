// Lua event listener support
#pragma once

#include <lualib.h>
#include <stdint.h>

// Event types for Lua listeners
typedef enum {
    LUA_EVENT_LEVEL_START,
    LUA_EVENT_LEVEL_LOAD,
    LUA_EVENT_PICKUP,
    LUA_EVENT_CONTROL_PRE,
    LUA_EVENT_CONTROL_POST,
} LUA_EVENT_TYPE;

// Initialize event API in Lua state
void LUA_CreateEvents(lua_State *L);

// Clear all listeners declared during the current level script
void Lua_ClearLevelListeners(void);

// Fire a Lua event of given type with integer argument
void Lua_FireEvent(LUA_EVENT_TYPE ev, int32_t arg);
