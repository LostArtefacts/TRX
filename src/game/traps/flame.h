#ifndef T1M_GAME_TRAPS_FLAME_H
#define T1M_GAME_TRAPS_FLAME_H

#include "global/types.h"

#include <stdint.h>

#define FLAME_ONFIRE_DAMAGE 5
#define FLAME_TOONEAR_DAMAGE 3

void SetupFlameEmitter(OBJECT_INFO *obj);
void SetupFlame(OBJECT_INFO *obj);
void FlameEmitterControl(int16_t item_num);
void FlameControl(int16_t fx_num);

#endif
