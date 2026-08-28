#pragma once

#include <trx/game/output/mesh_batcher/mesh.h>
#include <trx/game/output/scene_source.h>
#include <trx/game/output/uniforms.h>
#include <trx/game/rooms/types.h>
#include <trx/game/viewport.h>

#include <stdint.h>

typedef struct MESH_INSTANCE {
    OUTPUT_MESH *mesh;

    // TODO: use gl_InstanceID some day for this
    // and glMultiDrawArraysIndirect
    MATRIX cwmatrix;
    MATRIX wmatrix;
    const ROOM *room;
    RGBA_F tint;
    // Sets the water surface height so only the submerged part of the
    // instance is tinted; without a surface, the whole instance is tinted.
    bool has_water_line;
    float water_line;
    // Shifts the ambient light of the submerged part by this much, in place
    // of tinting it.
    bool has_submerged_ambient;
    RGB_F submerged_ambient_delta;
    // Sets the ambient light at the far end of an instance spanning two
    // rooms; without it, the instance uses one ambient light.
    bool has_ambient_span;
    RGB_F ambient_span_from;
    bool wibble;
    // Draws the instance a second time without the distortion, under the
    // distorted one and without writing depth. Faces that distort next to
    // faces that do not crack apart at the seam, and the plain copy fills
    // the cracks. An instance that distorts as a whole needs no fill.
    bool wibble_fill;
    int32_t water_effect;

    // Where the instance sits among the sorted pass's layers, drawn low first
    // and by depth within a layer. A shadow lies flat under the item it
    // belongs to, so no depth key puts it reliably behind every part of a body
    // resting on it; it takes a layer of its own instead.
    int32_t sort_layer;

    OUTPUT_LIGHT_INFO light_info;

    bool enable_scissor;
    float depth_adjust;
    VIEWPORT_RECT scissor;
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
