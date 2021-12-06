#ifndef T1M_GAME_RANDOM_H
#define T1M_GAME_RANDOM_H

#include <stdint.h>

void Random_SeedControl(int32_t seed);
void Random_SeedDraw(int32_t seed);
int32_t Random_GetControl();
int32_t Random_GetDraw();

#endif
