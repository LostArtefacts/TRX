#pragma once

#include <trx/core/math/types.h>

XYZ_32 XYZ_32_From16(XYZ_16 src);
int16_t XYZ_32_GetYaw(XYZ_32 pos);
int16_t XYZ_32_GetYawDiff(XYZ_32 pos1, const XYZ_32 pos2);
int16_t XYZ_32_GetPitch(XYZ_32 pos);
int32_t XYZ_32_GetDistance(XYZ_32 pos1, XYZ_32 pos2);

// Take length of a vector
int32_t XYZ_32_GetLength(XYZ_32 pos);

// Take squared length of a vector
int32_t XYZ_32_GetLength2(XYZ_32 pos);
int64_t XYZ_32_GetLength2_64(XYZ_32 pos);

int64_t XYZ_32_DotProduct_64(XYZ_32 a, XYZ_32 b);

bool XYZ_32_AreEquivalent(XYZ_32 pos1, XYZ_32 pos2);
bool XYZ_32_IsNearby(XYZ_32 pos1, XYZ_32 pos2, int32_t distance);

XYZ_32 XYZ_32_Add(XYZ_32 a, XYZ_32 b);
XYZ_32 XYZ_32_Subtract(XYZ_32 a, XYZ_32 b);

// An offset is nearly always known along the item it belongs to - z the way it
// faces, x out to its right - while the world it has to land in is not turned
// that way at all. A gun muzzle sits a fixed step ahead of Lara and a little to
// one side whichever way she is looking; where that is in the world depends on
// her yaw.
//
//     along the item, S                    the same, in the world
//
//         ahead (z)                          world z
//             ^                                 ^        P
//             |    P                            |      /
//             |   /                             |    /
//             | /                               |  /
//      -------S------> right (x)                S ----> ahead
//             |                                 | \ yaw is the turn from
//             |                                 |  \_ world z to ahead
//                                               +---------> world x
//
// Rotate turns an offset the item gave into the world's axes; Unrotate reads a
// world delta back along the item's, so its z says how far ahead of the item
// that delta reaches and its x how far to the side. The Yaw pair only turns
// about Y, which is all most items need; an item that pitches or rolls - a
// boat, a vehicle on a slope - needs the XYZ_16 pair to be right.
XYZ_32 XYZ_32_RotateYaw(XYZ_32 vec, int16_t yaw);
XYZ_32 XYZ_32_UnrotateYaw(XYZ_32 vec, int16_t yaw);
XYZ_32 XYZ_32_Rotate(XYZ_32 vec, XYZ_16 rot);
XYZ_32 XYZ_32_Unrotate(XYZ_32 vec, XYZ_16 rot);

XYZ_32 XYZ_32_FromYawPitch(int16_t yaw, int16_t pitch, int32_t distance);

// P itself rather than the turned offset that reaches it, for the callers that
// have S to hand. OffsetYaw is the common case of an offset straight ahead:
// XYZ_32_OffsetYaw(s, yaw, dist) is OffsetLocalYaw(s, { .z = dist }, yaw).
XYZ_32 XYZ_32_OffsetYaw(XYZ_32 src, int16_t yaw, int32_t distance);
XYZ_32 XYZ_32_OffsetLocalYaw(XYZ_32 src, XYZ_32 offset, int16_t yaw);
XYZ_32 XYZ_32_OffsetLocal(XYZ_32 src, XYZ_32 offset, XYZ_16 rot);

bool XYZ_32_ProjectPointOntoAxis(
    XYZ_32 origin, XYZ_32 axis, int64_t axis_len2, XYZ_32 *pos);

bool XYZ_16_AreEquivalent(XYZ_16 rot1, XYZ_16 rot2);

float XYZ_F_DotProduct(XYZ_F a, XYZ_F b);
float XYZ_F_Length2(XYZ_F pos);
float XYZ_F_Length(XYZ_F pos);
XYZ_F XYZ_F_Subtract(XYZ_F a, XYZ_F b);
