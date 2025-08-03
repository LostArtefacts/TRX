#pragma once

#include "../objects/types.h"
#include "../rooms.h"
#include "../viewport.h"

extern bool Output_Init(void);
extern void Output_Shutdown(void);
extern bool Output_IsHeadless(void);

extern void Output_DispatchLevelLoad(void);
extern void Output_DispatchLevelUnload(void);
extern void Output_DispatchRoomFlip(const ROOM *room);
extern void Output_DispatchObjectMeshUpdate(const OBJECT_MESH *mesh);
extern void Output_DispatchObjectMeshSwap(
    const OBJECT_MESH *mesh_1, const OBJECT_MESH *mesh_2);

extern void Output_BeginScene(void);
extern void Output_EndScene(void);
extern void Output_Flush(void);
extern void Output_FlipScreen(void);
extern bool Output_MakeScreenshot(const char *path);

extern int32_t Output_GetObjectBounds(const BOUNDS_16 *bounds);

// Temporary
extern void Output_DrawPolyList(void);
extern int32_t Output_CalcFogShade(int32_t depth);
extern int32_t Output_GetRoomLightShade(ROOM_LIGHT_MODE mode);
extern void Output_LightRoomVertices(const ROOM *room);

void Output_InitLight(void);
void Output_ShutdownLight(void);
void Output_CalculateLight(XYZ_32 pos, int16_t room_num);
void Output_CalculateStaticLight(int16_t adder);
void Output_CalculateStaticMeshLight(XYZ_32 pos, SHADE shade, const ROOM *room);
void Output_CalculateObjectLighting(const ITEM *item, const BOUNDS_16 *bounds);
void Output_LightRoom(ROOM *room);

void Output_ResetDynamicLights(void);
void Output_AddDynamicLight(XYZ_32 pos, int32_t intensity, int32_t falloff);

extern void Output_SwitchViewport(VIEWPORT_SPACE space);
extern void Output_ApplyLevelSettings(void);
