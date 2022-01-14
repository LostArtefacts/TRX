#pragma once

#include "global/types.h"

#include <stdint.h>

void InitialiseStartInfo();
void ModifyStartInfo(int32_t level_num);
void CreateStartInfo(int level_num);

int16_t SaveGame_LoadSaveBufferFromFile(GAME_INFO *save, int32_t slot);
void SaveGame_ApplySaveBuffer(GAME_INFO *save);

bool SaveGame_SaveToFile(GAME_INFO *save, int32_t slot);
void SaveGame_ScanSavedGames();
