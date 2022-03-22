#pragma once

#include "global/types.h"

#include <stdint.h>

#define PENDULUM_DAMAGE 100

void Pendulum_Setup(OBJECT_INFO *obj);
void Pendulum_Control(int16_t item_num);
