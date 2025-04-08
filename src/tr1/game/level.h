#pragma once

#include "game/game_flow/types.h"

#include <libtrx/colors.h>

bool Level_Initialise(const GF_LEVEL *level, GF_SEQUENCE_CONTEXT seq_ctx);
void Level_Load(const GF_LEVEL *level);

RGB_888 Level_GetWaterColor(void);
float Level_GetFogStart(void);
float Level_GetFogEnd(void);
