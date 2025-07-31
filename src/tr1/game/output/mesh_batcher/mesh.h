#pragma once

#include "game/output/textures.h"

#include <libtrx/game/matrix.h>
#include <libtrx/game/objects/types.h>
#include <libtrx/game/rooms/types.h>
#include <libtrx/game/types.h>
#include <libtrx/vector.h>

typedef struct {
    // attribute 2
    OUTPUT_UVW uvw;
    // attribute 3
    OUTPUT_TEXTURE_SIZE texture_size;
    // attribute 4
    float trapezoid_ratio[2];
} OUTPUT_MESH_TEXTURE;

typedef struct {
    XYZ_F pos;
    XYZ_F normal;
    uint16_t flags;
    int16_t uvw_idx;
    float trapezoid_ratio[2];
    int16_t shade;
    RGBA_8888 color;
} OUTPUT_MESH_VERTEX;

// Describes a contiguous block of vertices belonging to one face,
// with sort keys.
typedef struct {
    int32_t vertex_count;
    int32_t vertex_indices[6]; // Maximum 6 indices per face
    XYZ_F mesh_centroid;
} OUTPUT_MESH_FACE;

typedef struct {
    VECTOR *vertices;

    bool sealed;
    VECTOR *animated_vertices; // OUTPUT_VERTEX_RANGE
    VECTOR *transparent_faces; // OUTPUT_MESH_FACE

    VECTOR *opaque_vertex_indices; // int32_t
} OUTPUT_MESH;

OUTPUT_MESH *Output_Mesh_Create(void);
void Output_Mesh_Destroy(OUTPUT_MESH *mesh);

void Output_Mesh_AddRoomFace4(
    OUTPUT_MESH *mesh, const FACE4 *face, const ROOM *room);
void Output_Mesh_AddRoomFace3(
    OUTPUT_MESH *mesh, const FACE3 *face, const ROOM *room);
void Output_Mesh_AddRoomSprite(
    OUTPUT_MESH *mesh, const ROOM_SPRITE *room_sprite, const ROOM *room);

void Output_Mesh_AddObjectFace4(
    OUTPUT_MESH *mesh, const OBJECT_MESH *obj_mesh, const FACE4 *face,
    uint16_t flags);
void Output_Mesh_AddObjectFace3(
    OUTPUT_MESH *mesh, const OBJECT_MESH *obj_mesh, const FACE3 *face,
    uint16_t flags);

// Done streaming faces. This seals the batch (vertex_count is final), and
// immediately uploads the CPU‐side vertex & UV data to the GPU.
// (Later calls to UpdateTextures/UpdateShades will do incremental sub‑data.)
void Output_Mesh_Seal(OUTPUT_MESH *mesh);
