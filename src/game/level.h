#pragma once

#include "global/types.h"

#include <stdbool.h>
#include <stdint.h>

bool Level_Load(int32_t level_num);
bool Level_Initialise(int32_t level_num);
const LEVEL_INFO *Level_GetInfo(void);
