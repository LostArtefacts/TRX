#pragma once

#include "global/types.h"

void Math_GetVectorAngles(int32_t x, int32_t y, int32_t z, int16_t *dest);
int32_t Math_AngleInCone(int32_t angle1, int32_t angle2, int32_t cone);
int32_t Math_AngleMean(int32_t angle1, int32_t angle2, double ratio);
int32_t XYZ_32_GetDistance(const XYZ_32 *pos1, const XYZ_32 *pos2);
