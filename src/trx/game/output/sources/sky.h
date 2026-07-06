#pragma once

#include <trx/core/colors.h>
#include <trx/game/matrix.h>

void OutputSource_Sky_Init(void);
void OutputSource_Sky_Shutdown(void);

// Schedules a flat sky layer quad pair for drawing in the background pass.
// The model matrix positions the layer relative to the camera. The color is
// in the OG 128-neutral scale, handled by the shader's overbright path.
void OutputSource_Sky_StageLayer(
    const MATRIX *wmatrix, RGB_888 color, bool additive);
