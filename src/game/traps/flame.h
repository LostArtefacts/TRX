#pragma once

#include "global/types.h"

#include <stdint.h>

#define FLAME_ONFIRE_DAMAGE 5
#define FLAME_TOONEAR_DAMAGE 3

void Flame_Setup(OBJECT_INFO *obj);
void Flame_Control(int16_t fx_num);

void FlameEmitter_Setup(OBJECT_INFO *obj);
void FlameEmitter_Control(int16_t item_num);
