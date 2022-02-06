#pragma once

#include "global/types.h"

#include <stdint.h>

void Stats_ObserveRoomsLoad();
void Stats_ObserveItemsLoad();
void Stats_CalculateStats();
int32_t Stats_GetPickups();
int32_t Stats_GetKillables();
int32_t Stats_GetSecrets();
void Stats_LevelEnd(int32_t level_num);
