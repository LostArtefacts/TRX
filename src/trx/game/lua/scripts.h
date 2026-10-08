#pragma once

#include <trx/core/result.h>
#include <trx/game/game_flow/types.h>

// Runs the per-game script (scripts/_game.lua), if the game ships one, and
// reports script failure.
RESULT LUA_RunGameScript(void);

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
