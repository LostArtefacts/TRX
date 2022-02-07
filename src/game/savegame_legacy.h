#pragma once

#include "filesystem.h"
#include "game/savegame_common.h"
#include "global/types.h"

#include <stdint.h>

// TombATI implementation of savegames.

char *SaveGame_Legacy_GetSaveFileName(int32_t slot);
bool SaveGame_Legacy_FillInfo(MYFILE *fp, SAVEGAME_INFO *info);
bool SaveGame_Legacy_LoadFromFile(MYFILE *fp, GAME_INFO *game_info);
void SaveGame_Legacy_SaveToFile(MYFILE *fp, GAME_INFO *game_info);
