#pragma once

#include "global/types.h"

#define SHARD_DAMAGE 30
#define ROCKET_DAMAGE 100
#define ROCKET_RANGE SQUARE(WALL_L) // = 1048576

void Missile_Setup(OBJECT_INFO *obj);
void Missile_Control(int16_t fx_num);
