#pragma once

#include "global/types.h"

#include <stdint.h>

void InitialiseStartInfo();
void ModifyStartInfo(int32_t level_num);
void CreateStartInfo(int level_num);

int32_t SaveGame_GetLevelNumber(int32_t slot_num);

bool SaveGame_Load(int32_t slot_num, GAME_INFO *game_info);
bool SaveGame_Save(int32_t slot_num, GAME_INFO *game_info);

void SaveGame_ScanSavedGames();
void SaveGame_Shutdown();
