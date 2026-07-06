#pragma once

#include <trx/game/objects/types.h>
#include <trx/game/output/mesh_batcher/batcher.h>
#include <trx/game/output/scene_source.h>
#include <trx/game/output/uniforms.h>
#include <trx/game/types.h>

// Render-policy overrides that other modules can register for individual
// object meshes (e.g. the skybox), so that the generic mesh pipeline stays
// ignorant of their specifics. Individual hooks may be null. When several
// policies are registered for one mesh, they apply in registration order:
// vertex_flags are OR'ed together, the first get_* hook returning true wins,
// and every adjust_light_info runs.
typedef struct {
    // Extra VERT_* flags to apply to every vertex of the mesh.
    uint16_t vertex_flags;
    // Override the scene pass for a face; return false to keep the default.
    bool (*get_face_pass)(const FACE *face, SCENE_PASS *pass);
    // Override a face vertex color; return false to keep the default.
    bool (*get_vertex_color)(
        const FACE *face, int32_t vertex_idx, RGBA_8888 *color);
    // Override a face vertex shade; return false to keep the default.
    bool (*get_vertex_shade)(
        const FACE *face, int32_t vertex_idx, int32_t *shade);
    // Adjust lighting when staging the mesh.
    void (*adjust_light_info)(OUTPUT_LIGHT_INFO *info);
} OUTPUT_OBJECT_MESH_POLICY;

void OutputSource_Objects_Init(MESH_BATCHER *batcher);
void OutputSource_Objects_AddMeshPolicy(
    int32_t mesh_idx, const OUTPUT_OBJECT_MESH_POLICY *policy);
// Removes every registration of this policy, for any mesh index.
void OutputSource_Objects_RemoveMeshPolicy(
    const OUTPUT_OBJECT_MESH_POLICY *policy);
void OutputSource_Objects_Shutdown(void);
void OutputSource_Objects_ObserveLevelLoad(void);
void OutputSource_Objects_ObserveLevelUnload(void);
void OutputSource_Objects_ObserveObjectMeshSwap(
    int32_t mesh_idx_1, int32_t mesh_idx_2);
void OutputSource_Objects_ObserveObjectMeshUpdate(int32_t mesh_idx);

void OutputSource_Objects_StageObjectMesh(const OBJECT_MESH *mesh);

const SCENE_SOURCE *OutputSource_Objects_GetSource(void);
