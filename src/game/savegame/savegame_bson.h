#pragma once

#include "game/savegame.h"
#include "global/types.h"
#include "shared/filesystem.h"

#include <stdbool.h>
#include <stdint.h>

// TR1X implementation of savegames.

char *Savegame_BSON_GetSaveFileName(int32_t slot);
bool Savegame_BSON_FillInfo(MYFILE *fp, SAVEGAME_INFO *info);
bool Savegame_BSON_LoadFromFile(MYFILE *fp, GAME_INFO *game_info);
bool Savegame_BSON_LoadOnlyResumeInfo(MYFILE *fp, GAME_INFO *game_info);
void Savegame_BSON_SaveToFile(MYFILE *fp, GAME_INFO *game_info);
bool Savegame_BSON_UpdateDeathCounters(MYFILE *fp, GAME_INFO *game_info);
