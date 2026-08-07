#pragma once

#include <stdint.h>

// How wide one draw is: a draw returns 0 to RANDOM_SPAN - 1.
#define RANDOM_SPAN 0x8000

void Random_SeedControl(int32_t seed);
void Random_SeedDraw(int32_t seed);

int32_t Random_GetControl(void);
int32_t Random_GetDraw(void);
int32_t Random_GetControlSeed(void);
int32_t Random_GetDrawSeed(void);

void Random_FreezeDraw(bool is_frozen);
