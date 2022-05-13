#pragma once

#include "global/types.h"

#include <stdbool.h>
#include <stdint.h>

bool Game_Start(int32_t level_num, GAMEFLOW_LEVEL_TYPE level_type);
int32_t Game_Stop(void);
int32_t Game_Loop(GAMEFLOW_LEVEL_TYPE level_type);

bool Game_Pause(void);
