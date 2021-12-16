#pragma once

#include "global/types.h"

#include <stdint.h>

int32_t StartGame(int32_t level_num, GAMEFLOW_LEVEL_TYPE level_type);
int32_t StopGame();
int32_t GameLoop(int32_t demo_mode);
int32_t LevelCompleteSequence(int32_t level_num);
void LevelStats(int32_t level_num);
int32_t S_LoadGame(SAVEGAME_INFO *save, int32_t slot);
int32_t S_SaveGame(SAVEGAME_INFO *save, int32_t slot);

void Game_ScanSavedGames();
