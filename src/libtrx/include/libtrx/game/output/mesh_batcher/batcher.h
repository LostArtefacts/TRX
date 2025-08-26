#pragma once

#include "../../viewport.h"
#include "../scene_source.h"
#include "./mesh.h"

typedef struct MESH_INSTANCE {
    OUTPUT_MESH *mesh;

    MATRIX matrix;
    RGB_F tint;
    bool wibble;

    // TODO: remove these
    int32_t ls_adder;
    int32_t ls_divider;
    XYZ_32 ls_vector_view;

    bool enable_scissor;
    bool disable_z_writes;
    float depth_adjust;
    VIEWPORT_RECT scissor;

    int32_t (*transparent_sort_func)(
        const struct MESH_INSTANCE *inst, const OUTPUT_MESH_FACE *face);
    void (*update_light_func)(struct MESH_INSTANCE *inst, void *user_data);
    void *update_light_func_data;
    bool water_effect; // helper for the update_light_func
} MESH_INSTANCE;

typedef struct MESH_BATCHER MESH_BATCHER;

MESH_BATCHER *MeshBatcher_Create(void);
void MeshBatcher_Destroy(struct MESH_BATCHER *batcher);
void MeshBatcher_AddMesh(struct MESH_BATCHER *batcher, OUTPUT_MESH *mesh);
void MeshBatcher_RemoveMesh(struct MESH_BATCHER *batcher, OUTPUT_MESH *mesh);
void MeshBatcher_Seal(MESH_BATCHER *batcher);

const SCENE_SOURCE *MeshBatcher_AsSource(const struct MESH_BATCHER *batcher);

void MeshBatcher_Stage(
    struct MESH_BATCHER *batcher, const MESH_INSTANCE *inst, SCENE_PASS pass);
void MeshBatcher_UpdateMeshGeometry(
    const struct MESH_BATCHER *batcher, const OUTPUT_MESH *mesh);
