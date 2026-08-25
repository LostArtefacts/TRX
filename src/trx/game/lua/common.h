#pragma once

#include <trx/config/option.h>
#include <trx/core/result.h>
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
    LUA_CONTEXT_NUMBER_OF,
} LUA_CONTEXT;

RESULT LUA_Init(void);
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

// Gives a game's scripts a require() of their own, in place of the one the
// standard library ships, which is gone by then along with every other way a
// script could reach the filesystem. A name carries the directory it lives in,
// so a call site says which file it means and no two directories compete:
//
//     require("tr1.my_module")     games/tr1/modules/my_module.lua
//     require("common.my_module")  %trx_dir%/modules/my_module.lua
//
// The rest of the name spells directories the way Lua does, with dots, so
// require("tr1.my_group.my_module") reaches tr1/modules/my_group/my_module.lua.
// The module runs once, every later call handed what the first one returned.
// "trx" is reserved: the engine's own modules are the global trx table. What
// the engine runs lives in scripts/ instead, out of reach of a name.
void LUA_InstallModRequire(lua_State *L);

// Lets go of what a level's scripts required, so the next level runs them
// again. A module a level required attached its listeners as the level's, and
// those go with the level; the module has to run for them to come back.
void LUA_DropLevelModules(lua_State *L);

// Runs the per-game script (scripts/_game.lua), if the game ships one.
void LUA_RunGameScript(void);

// Pushes what a setting holds, in the shape a script reads it as: a bool as a
// bool, a number as a number, and a color, an enum or a string as text.
void LUA_Config_PushOptionValue(lua_State *L, const CONFIG_OPTION *option);

// Drops the config watchers a level script set up, as the level's listeners
// are dropped. A watcher a game script set up stays.
void LUA_Config_ClearLevelWatchers(void);

// Let go of the outgoing level's script: what it set up hears about it, and
// then its listeners go. Level_Unload does this for a level change; a path that
// re-runs a script without unloading the level does it for itself. The event
// waits on a level script run being outstanding, so the unload that opens the
// first level of a session passes in silence.
void LUA_DropLevelScript(void);

// Run a level's script.
void LUA_RunLevelScript(const GF_LEVEL *level);

// Reload current level script and reset level-scoped listeners.
void LUA_ReloadLevelScript(void);
