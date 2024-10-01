#pragma once

#include <stdint.h>

void __cdecl Random_SeedControl(int32_t seed);
void __cdecl Random_SeedDraw(int32_t seed);
int32_t __cdecl Random_GetControl(void);
int32_t __cdecl Random_GetDraw(void);
void __cdecl Random_Seed(void);
