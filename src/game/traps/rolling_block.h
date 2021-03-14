#ifndef T1M_GAME_TRAPS_ROLLING_BLOCK_H
#define T1M_GAME_TRAPS_ROLLING_BLOCK_H

#include "game/types.h"

#include <stdint.h>

void SetupRollingBlock(OBJECT_INFO *obj);
void InitialiseRollingBlock(int16_t item_num);
void RollingBlockControl(int16_t item_num);

#endif
