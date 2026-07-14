#pragma once

#include <trx/game/game_flow/types.h>

#include <lualib.h>
#include <stdint.h>

// Result of evaluating a Lua chunk.
typedef struct {
    int32_t code; // LUA_OK, LUA_ERRSYNTAX, LUA_ERRRUN, etc.
    char *message; // Error text (nullptr if code == LUA_OK).
} LUA_RESULT;

typedef enum {
    LUA_CONTEXT_GLOBAL,
    LUA_CONTEXT_LEVEL,
} LUA_CONTEXT;

void LUA_Init(void);
void LUA_Shutdown(void);

// Prints the full public API surface as JSON: the C-side FIELD_DESC tables plus
// the Lua-side trx.api registry. Used by --dump-lua-api.
void LUA_DumpAPI(void);

// Set script context: level script vs global script
LUA_CONTEXT LUA_GetScriptContext(void);
void LUA_SetScriptContext(LUA_CONTEXT context);

// Evaluate a Lua code string. Caller must free the result with LUA_FreeResult.
LUA_RESULT LUA_Eval(const char *code);

// Free the LUA eval result.
void LUA_FreeResult(LUA_RESULT *result);

// Evaluate a Lua script file. Caller must free the result with LUA_FreeResult.
LUA_RESULT LUA_EvalFile(const char *path);

// Run a level's script, having dropped what the last run of one left behind.
void LUA_RunLevelScript(const GF_LEVEL *level);

// Reload current level script and reset level-scoped listeners.
void LUA_ReloadLevelScript(void);
