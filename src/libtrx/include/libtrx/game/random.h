#pragma once

#include <stdint.h>

extern void Random_SeedControl(int32_t seed);
extern void Random_SeedDraw(int32_t seed);
extern int32_t Random_GetControl(void);
extern int32_t Random_GetDraw(void);
