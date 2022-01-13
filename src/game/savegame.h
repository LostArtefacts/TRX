#pragma once

#include "global/types.h"

#include <stdint.h>

void InitialiseStartInfo();
void ModifyStartInfo(int32_t level_num);
void CreateStartInfo(int level_num);

void CreateSaveGameInfo();
void ExtractSaveGameInfo();

bool SaveGame_LoadFromFile(SAVEGAME_INFO *save, int32_t slot);
bool SaveGame_SaveToFile(SAVEGAME_INFO *save, int32_t slot);
void SaveGame_ScanSavedGames();
