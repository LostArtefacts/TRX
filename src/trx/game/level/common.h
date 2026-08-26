#pragma once

#include <trx/core/result.h>
#include <trx/game/game_flow/types.h>

void Level_Unload(void);
RESULT Level_Initialise(const GF_LEVEL *level, GF_SEQUENCE_CONTEXT seq_ctx);

// Whether the level has its objects, rooms and items in memory. False while a
// level script runs, which happens before the level is read, and false again
// once the level is unloaded. Not the same as Game_IsLoaded, which reports
// which level is current rather than whether it is there to be read.
bool Level_IsWorldLoaded(void);
