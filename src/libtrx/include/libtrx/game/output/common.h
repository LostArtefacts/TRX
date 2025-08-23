#pragma once

#include "../objects/types.h"
#include "../rooms.h"
#include "../viewport.h"
#include "./shader.h"

void Output_Init(void);
void Output_Shutdown(void);
bool Output_IsHeadless(void);
OUTPUT_SHADER *Output_GetMeshShader(void);

void Output_BeginScene(void);
void Output_EndScene(void);
void Output_Flush(void);
void Output_FlipScreen(void);

void Output_SwitchViewport(VIEWPORT_SPACE space);

void Output_ApplyRenderSettings(void);
void Output_ApplyLevelSettings(void);

void Output_DispatchLevelLoad(void);
void Output_DispatchLevelUnload(void);
void Output_DispatchRoomFlip(const ROOM *room);
void Output_DispatchObjectMeshUpdate(int32_t mesh_idx);
void Output_DispatchObjectMeshSwap(int32_t mesh_idx_0, int32_t mesh_idx_1);
