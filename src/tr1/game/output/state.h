#pragma once

#include <libtrx/game/output/state.h>
#include <libtrx/gfx/context.h>

void Output_SetSkyboxEnabled(bool enabled);
bool Output_IsSkyboxEnabled(void);

void Output_SetWaterColor(RGB_888 color);
void Output_SetupBelowWater(bool underwater);
void Output_SetupAboveWater(bool underwater);

void Output_AnimateTextures(int32_t num_frames);
