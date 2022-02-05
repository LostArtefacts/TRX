#pragma once

#include "filesystem.h"
#include "global/types.h"

#include <stdint.h>

// Tomb1Main implementation of savegames.

char *SaveGame_BSON_GetSavePath(int32_t slot);

int16_t SaveGame_BSON_GetLevelNumber(MYFILE *fp);
int32_t SaveGame_BSON_GetSaveCounter(MYFILE *fp);
char *SaveGame_BSON_GetLevelTitle(MYFILE *fp);

bool SaveGame_BSON_ApplySaveBuffer(GAME_INFO *game_info);
void SaveGame_BSON_FillSaveBuffer(GAME_INFO *game_info);
