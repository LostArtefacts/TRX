#pragma once

#include <trx/core/utils.h>

#include <lualib.h>

// The C side of a trx.* module: the trxc table it builds for the Lua
// declaration to draw on, and what it lets go of when the state closes.
//
// A module registers itself, so LUA_Init drives no list and a module's create
// function stays private to its own file. The order they are created in is not
// guaranteed and nothing may depend on it: each writes its own trxc field and
// reads none of the others.
typedef struct {
    void (*create)(lua_State *L);
    void (*shutdown)(void);
} LUA_CAPI;

void LUA_Registry_Add(LUA_CAPI capi);
void LUA_Registry_CreateAll(lua_State *L);
void LUA_Registry_ShutdownAll(void);

#define REGISTER_LUA_CAPI(...)                                                 \
    __attribute__((__constructor__)) static void CONCAT(                       \
        M_RegisterLuaCAPI_, __LINE__)(void)                                    \
    {                                                                          \
        LUA_Registry_Add((LUA_CAPI) { __VA_ARGS__ });                          \
    }
