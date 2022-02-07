#pragma once

#include "global/types.h"

#include <stdint.h>

// Loading a saved game is divided into two phases. First, the game reads the
// savegame file contents to look for the level number. The rest of the save
// data is stored in a special buffer in the g_GameInfo. Then the engine
// continues to execute the normal game flow and loads the specified level.
// Second phase occurs after everything finishes loading, e.g. items,
// creatures, triggers etc., and is what actually sets Lara's health, creatures
// status, triggers, inventory etc.

void InitialiseStartInfo();
void ResetStartInfo(int32_t level_num);
void CreateStartInfo(int level_num);
void ModifyStartInfo(int32_t level_num);

int32_t SaveGame_GetLevelNumber(int32_t slot_num);

bool SaveGame_Load(int32_t slot_num, GAME_INFO *game_info);
bool SaveGame_Save(int32_t slot_num, GAME_INFO *game_info);

void SaveGame_ScanSavedGames();
void SaveGame_Shutdown();
