#pragma once

#include "../objects/types.h"
#include "../rooms.h"
#include "../viewport.h"
#include "./shader.h"

extern void Output_Init(void);
extern void Output_Shutdown(void);
bool Output_IsHeadless(void);

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

// Temporary
extern int32_t Output_CalcFogShade(int32_t depth);
extern int32_t Output_GetRoomLightShade(ROOM_LIGHT_MODE mode);
extern void Output_LightRoomVertices(const ROOM *room);

extern void Output_SwitchViewport(VIEWPORT_SPACE space);
extern void Output_ApplyLevelSettings(void);

extern OUTPUT_SHADER *Output_GetMeshShader(void);
