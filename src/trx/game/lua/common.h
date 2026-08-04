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

// Runs the per-game script (scripts/_game.lua), if the game ships one.
void LUA_RunGameScript(void);

// Let go of the outgoing level's script: what it set up hears about it, and
// then its listeners go. Level_Unload does this for a level change; a path that
// re-runs a script without unloading the level does it for itself. The event
// waits on a level script run being outstanding, so the unload that opens the
// first level of a session passes in silence.
void LUA_DropLevelScript(void);

// Whether the level scripts being run are probes rather than levels being
// played. The stats scan runs every level's script to count what the level
// holds; a probe's listeners go the way any other level script's do, and
// nothing is told about it, no level having been played.
void LUA_SetLevelScriptProbing(bool probing);

// Run a level's script.
void LUA_RunLevelScript(const GF_LEVEL *level);

// Reload current level script and reset level-scoped listeners.
void LUA_ReloadLevelScript(void);
