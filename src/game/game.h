#pragma once

#include "global/types.h"

#include <stdbool.h>
#include <stdint.h>

GAME_STATUS Game_GetStatus(void);
void Game_SetStatus(GAME_STATUS stauts);

bool Game_Start(int32_t level_num, GAMEFLOW_LEVEL_TYPE level_type);
int32_t Game_Stop(void);
int32_t Game_Loop(GAMEFLOW_LEVEL_TYPE level_type);

void Game_DisplayPicture(const char *path, double display_time);
void Game_ProcessInput(void);

int32_t Game_Cutscene_Start(int32_t level_num);
int32_t Game_Cutscene_Stop(int32_t level_num);
int32_t Game_Cutscene_Loop(void);

void Game_Demo(void);
bool Game_Demo_ProcessInput(void);
bool Game_Pause(void);

void Game_DrawScene(bool draw_overlay);
