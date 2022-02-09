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

#include "game/savegame_common.h"

void Savegame_InitStartEndInfo();

int32_t Savegame_GetLevelNumber(int32_t slot_num);

bool Savegame_Load(int32_t slot_num, GAME_INFO *game_info);
bool Savegame_Save(int32_t slot_num, GAME_INFO *game_info);

void Savegame_ScanSavedGames();
void Savegame_Shutdown();
