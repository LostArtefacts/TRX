#pragma once

#include <stdint.h>

void Math_GetVectorAngles(int32_t x, int32_t y, int32_t z, int16_t *dest);
int32_t Math_AngleInCone(int32_t angle1, int32_t angle2, int32_t cone);
int32_t Math_AngleMean(int32_t angle1, int32_t angle2, double ratio);
