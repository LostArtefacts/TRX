#pragma once

#include "filesystem.h"
#include "game/savegame_common.h"
#include "global/types.h"

#include <stdint.h>

// Tomb1Main implementation of savegames.

char *SaveGame_BSON_GetSaveFileName(int32_t slot);
bool SaveGame_BSON_FillInfo(MYFILE *fp, SAVEGAME_INFO *info);
bool SaveGame_BSON_LoadFromFile(MYFILE *fp, GAME_INFO *game_info);
void SaveGame_BSON_SaveToFile(MYFILE *fp, GAME_INFO *game_info);
