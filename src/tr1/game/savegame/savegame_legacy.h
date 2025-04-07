#pragma once

#include "game/savegame.h"
#include "global/types.h"

#include <libtrx/filesystem.h>

#include <stdint.h>

// TombATI implementation of savegames.

const char *Savegame_Legacy_GetSaveFilePattern(void);
bool Savegame_Legacy_FillInfo(MYFILE *fp, SAVEGAME_INFO *info);
bool Savegame_Legacy_LoadFromFile(MYFILE *fp, GAME_INFO *game_info);
bool Savegame_Legacy_LoadOnlyResumeInfo(MYFILE *fp, GAME_INFO *game_info);
