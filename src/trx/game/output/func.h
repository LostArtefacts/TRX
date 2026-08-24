#pragma once

#include <trx/game/output/types.h>

#include <stdint.h>

CLIP Output_CheckBoundsClip(const BOUNDS_16 *bounds);

void Output_MakeScreenshot(const char *path);

void Output_ApplyFOV(void);

// Returns the presented frames per second, counted against the wall clock.
int32_t Output_GetMeasuredFPS(void);
