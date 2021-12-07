#pragma once

#include "global/types.h"

#include <stdint.h>

#define PENDULUM_DAMAGE 100

void SetupPendulum(OBJECT_INFO *obj);
void PendulumControl(int16_t item_num);
