#pragma once

#include "global/types.h"

#include <stdint.h>

#define FALLING_CEILING_DAMAGE 300

void FallingCeiling_Setup(OBJECT_INFO *obj);
void FallingCeiling_Control(int16_t item_num);
