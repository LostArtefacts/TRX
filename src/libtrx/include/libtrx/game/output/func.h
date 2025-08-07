#pragma once

#include "./types.h"

CLIP Output_CheckBoundsClip(const BOUNDS_16 *bounds);

void Output_MakeScreenshot(const char *path);

void Output_ApplyFOV(void);

int32_t Output_CalcFogShade(int32_t depth);
