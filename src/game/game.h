#pragma once

#include "global/types.h"

#include <stdint.h>

bool StartGame(int32_t level_num, GAMEFLOW_LEVEL_TYPE level_type);
int32_t StopGame();
int32_t GameLoop(GAMEFLOW_LEVEL_TYPE level_type);
