#pragma once

#include "filesystem.h"
#include "global/types.h"

#include <stdint.h>

// TombATI implementation of savegames.

char *SaveGame_Legacy_GetSavePath(int32_t slot);

int16_t SaveGame_Legacy_GetLevelNumber(MYFILE *fp);
int32_t SaveGame_Legacy_GetSaveCounter(MYFILE *fp);
char *SaveGame_Legacy_GetLevelTitle(MYFILE *fp);

bool SaveGame_Legacy_LoadFromFile(MYFILE *fp, GAME_INFO *game_info);
void SaveGame_Legacy_SaveToFile(MYFILE *fp, GAME_INFO *game_info);
