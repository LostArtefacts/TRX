#pragma once

#include <stdint.h>

extern void Output_SetupBelowWater(bool is_underwater);
extern void Output_SetupAboveWater(bool is_underwater);

int32_t Output_GetFogStart(void);
extern int32_t Output_GetFogEnd(void);

void Output_SetFogStart(int32_t dist);
extern void Output_SetFogEnd(int32_t dist);

extern int32_t Output_GetLightAdder(void);
extern void Output_SetLightAdder(int32_t adder);
extern void Output_SetLightDivider(int32_t divider);
extern void Output_RotateLight(int16_t pitch, int16_t yaw);
