#pragma once

#include <stdint.h>

void Random_SeedControl(int32_t seed);
void Random_SeedDraw(int32_t seed);
int32_t Random_GetControl(void);
int32_t Random_GetDraw(void);
