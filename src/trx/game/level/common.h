#pragma once

#include <trx/core/result.h>
#include <trx/game/game_flow/types.h>

void Level_Unload(void);
RESULT Level_Initialise(const GF_LEVEL *level, GF_SEQUENCE_CONTEXT seq_ctx);
