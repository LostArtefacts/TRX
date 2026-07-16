#pragma once

#include <trx/core/math/types.h>

#include <stdint.h>

// TR4 fires. A single 20-spark pool is simulated in local space and drawn at
// every fire registered this frame, scaled and dimmed per registration. The
// registry is rebuilt each tick: FX_Fire_NewFrame clears it before object
// control runs, emitters re-add through FX_Fire_Add, and the draw only reads.

void FX_Fire_Reset(void);
void FX_Fire_NewFrame(void);
void FX_Fire_Control(void);
void FX_Fire_Draw(void);

// Registers a fire at an absolute world position for this frame. size is 0
// (small), 1 (medium) or 2 (big); fade dims the fire, 0 meaning full strength.
void FX_Fire_Add(XYZ_32 pos, int32_t size, int16_t room_num, int32_t fade);
