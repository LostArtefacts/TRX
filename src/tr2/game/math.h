#pragma once

#include "global/types.h"

#include <stdint.h>

int32_t __fastcall Math_Cos(int32_t angle);
int32_t __fastcall Math_Sin(int32_t angle);
int32_t __fastcall Math_Atan(int32_t x, int32_t y);
uint32_t __fastcall Math_Sqrt(uint32_t n);

DIRECTION Math_GetDirection(int16_t angle);
DIRECTION Math_GetDirectionCone(int16_t angle, int16_t cone);
int16_t Math_DirectionToAngle(DIRECTION dir);

int32_t XYZ_32_GetDistance(const XYZ_32 *pos1, const XYZ_32 *pos2);
int32_t XYZ_32_GetDistance0(const XYZ_32 *pos);
