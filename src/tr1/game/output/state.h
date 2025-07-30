#pragma once

#include <libtrx/game/output/state.h>
#include <libtrx/gfx/context.h>

int32_t Output_GetTime(void);

void Output_SetSkyboxEnabled(bool enabled);
bool Output_IsSkyboxEnabled(void);

int32_t Output_GetNearZ(void);
int32_t Output_GetFarZ(void);
int32_t Output_GetNearZ_UI(void);
int32_t Output_GetFarZ_UI(void);

RGB_F Output_GetTint(void);
bool Output_GetWaterEffect(void);
bool Output_GetWibbleEffect(void);

void Output_SetWaterColor(RGB_888 color);
void Output_SetupBelowWater(bool underwater);
void Output_SetupAboveWater(bool underwater);

int32_t Output_GetLightDivider(void);
XYZ_32 Output_GetLightVectorView(void);

void Output_AnimateTextures(int32_t num_frames);

void Output_GetPerspProjectionMatrix(GLfloat output[][4]);
void Output_GetOrthoProjectionMatrix(GLfloat output[][4]);
void Output_EnableScissor(float x, float y, float w, float h);
void Output_DisableScissor(void);
