#pragma once

#include "global/types.h"

#include <stdbool.h>
#include <stdint.h>

GAME_STATUS Game_GetStatus(void);
void Game_SetStatus(GAME_STATUS stauts);

bool Game_Start(int32_t level_num, GAMEFLOW_LEVEL_TYPE level_type);
GAMEFLOW_OPTION Game_Stop(void);
GAMEFLOW_OPTION Game_Loop(void);

void Game_ProcessInput(void);

GAMEFLOW_OPTION Game_Cutscene_Start(int32_t level_num);

void Game_Demo(void);
bool Game_Demo_ProcessInput(void);

void Game_DrawScene(bool draw_overlay);
