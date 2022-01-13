#pragma once

#include "global/types.h"

#include <stdint.h>

void InitialiseStartInfo();
void ModifyStartInfo(int32_t level_num);
void CreateStartInfo(int level_num);

void SaveGame_SaveToSave(SAVEGAME_INFO *save);
void SaveGame_LoadFromSave(SAVEGAME_INFO *save);

bool SaveGame_LoadFromFile(SAVEGAME_INFO *save, int32_t slot);
bool SaveGame_SaveToFile(SAVEGAME_INFO *save, int32_t slot);
void SaveGame_ScanSavedGames();
