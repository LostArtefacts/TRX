#pragma once

#include <trx/core/math/types.h>

// The red dot of the laser sight, where the sight points. The laser sight
// view places it every tick while the view is up, and takes it away as the
// view closes.
void FX_LaserSight_SetDot(XYZ_32 pos);
void FX_LaserSight_ClearDot(void);
