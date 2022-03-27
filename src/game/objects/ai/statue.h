#pragma once

#include "global/types.h"

#include <stdint.h>

#define STATUE_EXPLODE_DIST (WALL_L * 7 / 2) // = 3584

void Statue_Setup(OBJECT_INFO *obj);
void Statue_Initialise(int16_t item_num);
void Statue_Control(int16_t item_num);
