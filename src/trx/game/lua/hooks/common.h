#pragma once

#include <trx/game/lua/utils/item.h>

#include <lua.h>
#include <stdint.h>

// A script function the engine asks in place of, or alongside, its own
// answer. Every hook is listed in hooks.def; a hook can also be named by a key,
// such as the object whose hit effect it decides, or by nothing at all.

typedef enum {
#define X_HOOK(id, name) id,
#include <trx/game/lua/hooks/hooks.def>
#undef X_HOOK
    LUA_HOOK_NUMBER_OF,
} LUA_HOOK_TYPE;

// One argument a hook is handed, with the function that pushes it.
typedef struct LUA_HOOK_ARG LUA_HOOK_ARG;
struct LUA_HOOK_ARG {
    void (*push)(lua_State *L, const LUA_HOOK_ARG *arg);
    union {
        bool b;
        int32_t i32;
        const void *ptr;
    } value;
};

// What a hook that keys or installs itself does as it is set and cleared.
typedef struct {
    // Reads the arguments a script names the hook by, from `arg` on, into a
    // key. Raises a Lua error where they name nothing.
    int32_t (*read_key)(lua_State *L, int arg);
    void (*on_set)(LUA_HOOK_TYPE hook, int32_t key);
    void (*on_clear)(LUA_HOOK_TYPE hook, int32_t key);
} LUA_HOOK_HANDLERS;

static inline void LUA_Hooks_PushBool(lua_State *L, const LUA_HOOK_ARG *arg)
{
    lua_pushboolean(L, arg->value.b);
}

static inline void LUA_Hooks_PushInt(lua_State *L, const LUA_HOOK_ARG *arg)
{
    lua_pushinteger(L, arg->value.i32);
}

static inline void LUA_Hooks_PushItem(lua_State *L, const LUA_HOOK_ARG *arg)
{
    LUA_PushItem(L, (int16_t)arg->value.i32);
}

#define LUA_ARG_BOOL(value_)                                                   \
    ((LUA_HOOK_ARG) { .push = LUA_Hooks_PushBool, .value.b = (value_) })
#define LUA_ARG_INT(value_)                                                    \
    ((LUA_HOOK_ARG) { .push = LUA_Hooks_PushInt, .value.i32 = (value_) })
#define LUA_ARG_ITEM(item_num_)                                                \
    ((LUA_HOOK_ARG) { .push = LUA_Hooks_PushItem, .value.i32 = (item_num_) })

void LUA_Hooks_SetHandlers(
    LUA_HOOK_TYPE hook, const LUA_HOOK_HANDLERS *handlers);

// The hook a script names by `name`, or LUA_HOOK_NUMBER_OF where none is.
LUA_HOOK_TYPE LUA_Hooks_GetByName(const char *name);

// Reads the arguments a script names the hook by, from `arg` on, into a key.
// Raises a Lua error where they name nothing. A hook named by nothing has the
// key 0.
int32_t LUA_Hooks_ReadKey(lua_State *L, LUA_HOOK_TYPE hook, int arg);

// Sets the function at `fn_idx` as the hook under a key, replacing the one
// set there; nil clears it. A hook set by a level script is cleared as the
// level ends.
void LUA_Hooks_Set(lua_State *L, LUA_HOOK_TYPE hook, int32_t key, int fn_idx);

bool LUA_Hooks_IsSet(LUA_HOOK_TYPE hook, int32_t key);

// Calls the hook set under a key with the given arguments, and returns the
// state with `result_count` results on it for the caller to read and pop.
// Returns nullptr where no hook is set, and where the hook fails, which is
// reported to the console.
lua_State *LUA_Hooks_CallEx(
    LUA_HOOK_TYPE hook, int32_t key, const LUA_HOOK_ARG *args,
    int32_t arg_count, int32_t result_count);

// Calls the hook for a yes or no, or a whole number, and returns `fallback`
// where no hook is set, the hook fails, or it hands back nothing.
bool LUA_Hooks_CallBoolEx(
    LUA_HOOK_TYPE hook, int32_t key, bool fallback, const LUA_HOOK_ARG *args,
    int32_t arg_count);
int32_t LUA_Hooks_CallIntEx(
    LUA_HOOK_TYPE hook, int32_t key, int32_t fallback, const LUA_HOOK_ARG *args,
    int32_t arg_count);

#define LUA_HOOK_ARGS(...)                                                     \
    (const LUA_HOOK_ARG[]) { __VA_ARGS__ },                                    \
        (int32_t)(sizeof((LUA_HOOK_ARG[]) { __VA_ARGS__ })                     \
                  / sizeof(LUA_HOOK_ARG))

#define LUA_Hooks_Call(hook, key, result_count, ...)                           \
    LUA_Hooks_CallEx(hook, key, LUA_HOOK_ARGS(__VA_ARGS__), result_count)
#define LUA_Hooks_CallBool(hook, key, fallback, ...)                           \
    LUA_Hooks_CallBoolEx(hook, key, fallback, LUA_HOOK_ARGS(__VA_ARGS__))
#define LUA_Hooks_CallInt(hook, key, fallback, ...)                            \
    LUA_Hooks_CallIntEx(hook, key, fallback, LUA_HOOK_ARGS(__VA_ARGS__))
