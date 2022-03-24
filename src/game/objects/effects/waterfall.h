#pragma once

#include "global/types.h"

#include <stdint.h>

#define WATERFALL_RANGE (WALL_L * 10) // = 10240

void Waterfall_Setup(OBJECT_INFO *obj);
void Waterfall_Control(int16_t item_num);
