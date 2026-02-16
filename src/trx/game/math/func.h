#pragma once

#include <trx/game/math/types.h>

int32_t Math_Cos(int32_t angle);
int32_t Math_Sin(int32_t angle);
int32_t Math_Atan(int32_t x, int32_t y);
uint32_t Math_Sqrt(uint32_t n);
uint64_t Math_Sqrt64(uint64_t n);

void Math_GetVectorAngles(int32_t x, int32_t y, int32_t z, int16_t *dest);
int32_t Math_AngleInCone(int32_t angle1, int32_t angle2, int32_t cone);
DIRECTION Math_GetDirection(int16_t angle);
DIRECTION Math_GetDirectionCone(int16_t angle, int16_t cone);
int16_t Math_DirectionToAngle(DIRECTION dir);
int32_t Math_AngleMean(int32_t angle1, int32_t angle2, double ratio);
int32_t Math_FloorDiv(int32_t x, int32_t divisor);
int32_t Math_GCD(int32_t a, int32_t b);

XYZ_32 XYZ_32_From16(XYZ_16 src);
int16_t XYZ_32_GetYaw(XYZ_32 pos);
int16_t XYZ_32_GetPitch(XYZ_32 pos);
int32_t XYZ_32_GetDistance(const XYZ_32 *pos1, const XYZ_32 *pos2);
int32_t XYZ_32_GetLength(XYZ_32 pos);
int64_t XYZ_32_GetLength2_64(XYZ_32 pos);
int64_t XYZ_32_DotProduct_64(XYZ_32 a, XYZ_32 b);
bool XYZ_32_AreEquivalent(const XYZ_32 *pos1, const XYZ_32 *pos2);
bool XYZ_32_IsNearby(const XYZ_32 *pos1, const XYZ_32 *pos2, int32_t distance);
XYZ_32 XYZ_32_FromYawPitch(int16_t yaw, int16_t pitch, int32_t distance);
XYZ_32 XYZ_32_OffsetYaw(XYZ_32 src, int16_t yaw, int32_t distance);
bool XYZ_32_ProjectPointOntoAxis(
    XYZ_32 origin, XYZ_32 axis, int64_t axis_len2, XYZ_32 *pos);

bool XYZ_16_AreEquivalent(const XYZ_16 *rot1, const XYZ_16 *rot2);

float XYZ_F_DotProduct(XYZ_F a, XYZ_F b);
float XYZ_F_Length2(XYZ_F pos);
float XYZ_F_Length(XYZ_F pos);
XYZ_F XYZ_F_Subtract(XYZ_F a, XYZ_F b);

bool Bounds32_Intersect(const BOUNDS_32 *a, const BOUNDS_32 *b);
