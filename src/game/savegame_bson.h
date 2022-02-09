#pragma once

#include "filesystem.h"
#include "game/savegame_common.h"
#include "global/types.h"

#include <stdint.h>

// Tomb1Main implementation of savegames.

char *Savegame_BSON_GetSaveFileName(int32_t slot);
bool Savegame_BSON_FillInfo(MYFILE *fp, SAVEGAME_INFO *info);
bool Savegame_BSON_LoadFromFile(MYFILE *fp, GAME_INFO *game_info);
void Savegame_BSON_SaveToFile(MYFILE *fp, GAME_INFO *game_info);
