#pragma once

#include <libtrx/game/output/state.h>
#include <libtrx/gfx/context.h>

void Output_SetSkyboxEnabled(bool enabled);
bool Output_IsSkyboxEnabled(void);

RGB_F Output_GetTint(void);
bool Output_GetWaterEffect(void);
bool Output_GetWibbleEffect(void);

void Output_SetWaterColor(RGB_888 color);
void Output_SetupBelowWater(bool underwater);
void Output_SetupAboveWater(bool underwater);

int32_t Output_GetLightDivider(void);
XYZ_32 Output_GetLightVectorView(void);

void Output_AnimateTextures(int32_t num_frames);
