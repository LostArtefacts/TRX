#ifndef T1M_GAME_TRAPS_PENDULUM_H
#define T1M_GAME_TRAPS_PENDULUM_H

#include "game/types.h"
#include <stdint.h>

#define PENDULUM_DAMAGE 100

void SetupPendulum(OBJECT_INFO *obj);
void PendulumControl(int16_t item_num);

#endif
