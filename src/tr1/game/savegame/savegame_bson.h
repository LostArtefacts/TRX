#pragma once

#include "game/savegame.h"
#include "global/types.h"

#include <libtrx/filesystem.h>

#include <stdint.h>

// TR1X implementation of savegames.

const char *Savegame_BSON_GetSaveFilePattern(void);
bool Savegame_BSON_FillInfo(MYFILE *fp, SAVEGAME_INFO *info);
bool Savegame_BSON_LoadFromFile(MYFILE *fp);
bool Savegame_BSON_LoadOnlyResumeInfo(MYFILE *fp);
void Savegame_BSON_SaveToFile(MYFILE *fp, SAVEGAME_INFO *savegame_info);
bool Savegame_BSON_UpdateDeathCounters(MYFILE *fp, int32_t death_count);
