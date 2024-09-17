#pragma once

#include "global/types.h"

#include <stdbool.h>
#include <stdint.h>

bool Game_Start(int32_t level_num, GAMEFLOW_LEVEL_TYPE level_type);
GAMEFLOW_COMMAND Game_Stop(void);

void Game_ProcessInput(void);

void Game_DrawScene(bool draw_overlay);

GAMEFLOW_COMMAND Game_MainMenu(void);

bool Game_IsPlayable(void);
