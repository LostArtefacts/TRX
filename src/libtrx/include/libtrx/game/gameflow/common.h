#pragma once

#include "./types.h"

extern int32_t Gameflow_GetLevelCount(void);
extern const char *Gameflow_GetLevelFileName(int32_t level_num);
extern const char *Gameflow_GetLevelTitle(int32_t level_num);
extern int32_t Gameflow_GetGymLevelNumber(void);

extern void Gameflow_OverrideCommand(GAMEFLOW_COMMAND action);
