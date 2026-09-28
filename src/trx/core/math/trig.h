#pragma once

#include <stdint.h>

int32_t Math_Cos(int32_t angle);
int32_t Math_Sin(int32_t angle);

// Switches sine and cosine to the coarser table of the TR3 engine, which
// resolves 4096 steps per turn to 1 in 4096 instead of 1 in 16384. Movement in
// TR3 is only exact with that table.
void Math_SetCoarseTrig(bool enable);
int32_t Math_Atan(int32_t x, int32_t y);
