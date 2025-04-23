#pragma once

#include "../rooms.h"

extern void Output_ObserveLevelLoad(void);
extern void Output_ObserveLevelUnload(void);
extern void Output_ObserveRoomFlip(const ROOM *room);

extern bool Output_MakeScreenshot(const char *path);
extern void Output_BeginScene(void);
extern void Output_EndScene(void);

extern void Output_DrawBlackRectangle(int32_t opacity);
extern void Output_DrawPolyList(void);

extern void Output_SetupBelowWater(bool is_underwater);
extern void Output_SetupAboveWater(bool is_underwater);
extern void Output_RotateLight(int16_t pitch, int16_t yaw);
extern void Output_SetLightAdder(int32_t adder);
extern int32_t Output_GetLightAdder(void);
extern void Output_SetLightDivider(int32_t divider);

extern int32_t Output_GetObjectBounds(const BOUNDS_16 *bounds);

// Temporary
extern int32_t Output_CalcFogShade(int32_t depth);
extern int32_t Output_GetRoomLightShade(ROOM_LIGHT_MODE mode);
extern void Output_LightRoomVertices(const ROOM *room);

void Output_CalculateLight(XYZ_32 pos, int16_t room_num);
void Output_CalculateStaticLight(int16_t adder);
void Output_CalculateStaticMeshLight(XYZ_32 pos, SHADE shade, const ROOM *room);
void Output_CalculateObjectLighting(const ITEM *item, const BOUNDS_16 *bounds);
void Output_LightRoom(ROOM *room);

void Output_ResetDynamicLights(void);
void Output_AddDynamicLight(XYZ_32 pos, int32_t intensity, int32_t falloff);

int32_t Output_GetFogStart(void);
void Output_SetFogStart(int32_t dist);
extern int32_t Output_GetFogEnd(void);
extern void Output_SetFogEnd(int32_t dist);
