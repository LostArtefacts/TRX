#pragma once

#include <stdint.h>

void Output_MakeScreenshot(const char *path);

void Output_ApplyFOV(void);

int32_t Output_CalcFogShade(int32_t depth);
