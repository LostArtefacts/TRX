#pragma once

#include <trx/game/camera/types.h>

#define SPLINE_ONE (1 << 16)

void Spline_SetupData(SPLINE_DATA *data, int32_t slot_idx, int32_t camera_idx);
int32_t Spline_Calculate(int32_t t, const int32_t *knots, int32_t knot_count);
int32_t Spline_GetNearestPosition(
    XYZ_32 pos, const SPLINE_DATA *data, int32_t spline_count);
