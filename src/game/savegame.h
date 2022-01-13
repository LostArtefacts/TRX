#pragma once

#include "global/types.h"

#include <stdint.h>

void InitialiseStartInfo();
void ModifyStartInfo(int32_t level_num);
void CreateStartInfo(int level_num);

void SaveGame_SaveToSave(GAME_INFO *save);
void SaveGame_LoadFromSave(GAME_INFO *save);

bool SaveGame_LoadFromFile(GAME_INFO *save, int32_t slot);
bool SaveGame_SaveToFile(GAME_INFO *save, int32_t slot);
void SaveGame_ScanSavedGames();
