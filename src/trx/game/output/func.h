#pragma once

#include <trx/game/output/types.h>

#include <stdint.h>

CLIP Output_CheckBoundsClip(const BOUNDS_16 *bounds);

void Output_MakeScreenshot(const char *path);

void Output_ApplyFOV(void);

// Returns the distance from the eye to a screen of the given width that the
// given horizontal field of view spans exactly.
int32_t Output_GetPerspective(int32_t width, int32_t fov);

// Returns the presented frames per second, counted against the wall clock.
int32_t Output_GetMeasuredFPS(void);
