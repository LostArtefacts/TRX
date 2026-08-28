#pragma once

#include <trx/core/result.h>
#include <trx/game/objects/types.h>
#include <trx/game/output/shaders/mesh.h>
#include <trx/game/output/shaders/ui.h>
#include <trx/game/output/uniforms.h>
#include <trx/game/rooms.h>
#include <trx/game/viewport.h>

RESULT Output_Init(void);
bool Output_IsHeadless(void);

const OUTPUT_UNIFORMS *Output_GetUniforms(void);
OUTPUT_MESH_SHADER *Output_GetMeshShader(void);
OUTPUT_UI_SHADER *Output_GetUIShader(void);

void Output_BeginScene(void);
void Output_EndScene(void);
void Output_Flush(void);
void Output_FlipScreen(void);

void Output_SwitchViewport(VIEWPORT_SPACE space);

// Suspends or restores supersampling and resizes the framebuffers to match.
// Call it between frames only.
void Output_SetSupersamplingEnabled(bool enabled);

void Output_ApplyRenderSettings(void);
void Output_ApplyLevelSettings(void);

void Output_DispatchLevelLoad(void);
void Output_DispatchLevelUnload(void);
// Rebakes the object meshes so that face data changes (e.g. semi-transparency
// flags) take effect without a level reload.
void Output_RefreshObjectMeshes(void);
void Output_DispatchRoomFlip(const ROOM *room);
void Output_DispatchObjectMeshUpdate(int32_t mesh_idx);
// Uploads new vertex positions for the mesh into the GPU buffer, for meshes
// whose geometry is deformed at runtime (e.g. Lara's skin joints). The array
// is indexed like the OBJECT_MESH's own vertices. A non-null normals array is
// indexed the same way and replaces the baked normals, letting a deformed seam
// take on the lighting of the meshes it welds to.
void Output_DispatchObjectMeshGeometry(
    int32_t mesh_idx, const XYZ_F *positions, const XYZ_F *normals,
    const float *tint_factors);
void Output_DispatchObjectMeshSwap(int32_t mesh_idx_0, int32_t mesh_idx_1);
