#pragma once

#include "game/game_flow/types.h"

#include <libtrx/game/level.h>

bool Level_Initialise(const GF_LEVEL *level, GF_SEQUENCE_CONTEXT seq_ctx);
void Level_Load(const GF_LEVEL *level);
