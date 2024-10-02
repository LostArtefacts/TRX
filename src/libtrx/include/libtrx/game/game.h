#pragma once

#include "./gameflow/types.h"

#include <stdbool.h>

extern bool Game_IsPlayable(void);
extern GAMEFLOW_LEVEL_TYPE Game_GetCurrentLevelType(void);
