#pragma once

#include "game/game_flow/types.h"

#include <libtrx/game/level/common.h>

bool Level_Initialise(const GF_LEVEL *level, GF_SEQUENCE_CONTEXT seq_ctx);
bool Level_Load(const GF_LEVEL *level);
void Level_Unload(void);
