#pragma once

// Warm up the shaders JIT by drawing dummy geometry, forcing the shader to run
// certain codepaths at the game startup rather than runtime to eliminate
// potential mid-play lags.
void Output_WarmUp(void);
