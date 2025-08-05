#pragma once

#include <GL/glew.h>
#include <stdint.h>

void Output_GetPerspProjectionMatrix(GLfloat output[][4]);
void Output_GetOrthoProjectionMatrix(GLfloat output[][4]);

extern int32_t Output_GetTime(void);

extern void Output_SetupBelowWater(bool is_underwater);
extern void Output_SetupAboveWater(bool is_underwater);

int32_t Output_GetFogStart(void);
extern int32_t Output_GetFogEnd(void);

void Output_SetFogStart(int32_t dist);
extern void Output_SetFogEnd(int32_t dist);

extern int32_t Output_GetNearZ(void);
extern int32_t Output_GetFarZ(void);
int32_t Output_GetNearZ_UI(void);
int32_t Output_GetFarZ_UI(void);

extern int32_t Output_GetLightAdder(void);
extern void Output_SetLightAdder(int32_t adder);
extern void Output_SetLightDivider(int32_t divider);
extern void Output_RotateLight(int16_t pitch, int16_t yaw);

extern void Output_EnableScissor(float x, float y, float w, float h);
extern void Output_DisableScissor(void);
