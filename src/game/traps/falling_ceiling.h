#ifndef T1M_GAME_TRAPS_FALLING_CEILING_H
#define T1M_GAME_TRAPS_FALLING_CEILING_H

#include "global/types.h"

#include <stdint.h>

#define FALLING_CEILING_DAMAGE 300

void SetupFallingCeilling(OBJECT_INFO *obj);
void FallingCeilingControl(int16_t item_num);

#endif
