#pragma once

#include "game/savegame.h"
#include "global/types.h"

#include <libtrx/filesystem.h>

#include <stdbool.h>
#include <stdint.h>

// TombATI implementation of savegames.

char *Savegame_Legacy_GetSaveFileName(int32_t slot);
bool Savegame_Legacy_FillInfo(MYFILE *fp, SAVEGAME_INFO *info);
bool Savegame_Legacy_LoadFromFile(MYFILE *fp, GAME_INFO *game_info);
bool Savegame_Legacy_LoadOnlyResumeInfo(MYFILE *fp, GAME_INFO *game_info);
void Savegame_Legacy_SaveToFile(MYFILE *fp, GAME_INFO *game_info);
bool Savegame_Legacy_UpdateDeathCounters(MYFILE *fp, GAME_INFO *game_info);
