#pragma once

#include "global/types.h"

#include <stdint.h>

#define WATERFALL_RANGE (WALL_L * 10) // = 10240

void SetupWaterfall(OBJECT_INFO *obj);
void ControlWaterFall(int16_t item_num);
