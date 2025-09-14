#pragma once

#include "../../colors.h"
#include "../math/types.h"

#include <GL/glew.h>

void Output_SetSkyboxEnabled(bool enabled);
bool Output_IsSkyboxEnabled(void);

void Output_GetPerspProjectionMatrix(GLfloat output[][4]);
void Output_GetOrthoProjectionMatrix(GLfloat output[][4]);

void Output_SetTime(float time);
float Output_GetTime(void);
float Output_GetTimeInGame(void);

void Output_SetupBelowWater(bool is_underwater);
void Output_SetupAboveWater(bool is_underwater);
void Output_SetWaterColor(RGB_888 color);

RGB_F Output_GetTint(void);
bool Output_GetWaterEffect(void);
bool Output_GetWibbleEffect(void);

RGBA_F Output_GetFogColor(void);
int32_t Output_GetFogStart(void);
int32_t Output_GetFogEnd(void);

void Output_SetFogColor(RGBA_8888 color);
void Output_SetFogStart(int32_t dist);
void Output_SetFogEnd(int32_t dist);

int32_t Output_GetNearZ(void);
int32_t Output_GetFarZ(void);
int32_t Output_GetNearZ_UI(void);
int32_t Output_GetFarZ_UI(void);

int32_t Output_GetLightAdder(void);
int32_t Output_GetLightDivider(void);
XYZ_32 Output_GetLightVectorView(void);
void Output_SetLightAdder(int32_t adder);
void Output_SetLightDivider(int32_t divider);
void Output_RotateLight(int16_t pitch, int16_t yaw);

void Output_EnableScissor(float x, float y, float w, float h);
void Output_DisableScissor(void);

void Output_AdjustDepth(float factor, float units);

void Output_SetupBelowWater(bool underwater);
void Output_SetupAboveWater(bool underwater);

void Output_AnimateTextures(int32_t num_frames);
