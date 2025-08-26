#pragma once

#include "../../rooms/types.h"
#include "./mesh.h"

// Opaque builder for incrementally constructing an OUTPUT_MESH.
typedef struct MESH_BUILDER MESH_BUILDER;

// Create a new mesh builder. Call MeshBuilder_Seal() when done, then
// MeshBuilder_Destroy().
MESH_BUILDER *MeshBuilder_Create(void);

// Destroy the builder state. Does NOT free the emitted OUTPUT_MESHes.
void MeshBuilder_Destroy(MESH_BUILDER *builder);

// Append one vertex to the mesh under construction.
// Must be called before adding faces for those vertices.
void MeshBuilder_AddVertex(
    MESH_BUILDER *builder, const OUTPUT_MESH_VERTEX *vertex);

// Add a face using the recently added vertices. transparent=true records it in
// the transparent face list, false in opaque indices.
void MeshBuilder_AddFace(
    MESH_BUILDER *builder, bool transparent, const int32_t *indices,
    size_t idx_count);

// Add a triangle face using the last three vertices added.
void MeshBuilder_AddFace3(MESH_BUILDER *builder, bool transparent);

// Add a quad face (split internally) using the last four vertices added.
void MeshBuilder_AddFace4(MESH_BUILDER *builder, bool transparent);

// Add a triangle fan face using the last vertices added: a center followed by
// ring vertices.
void MeshBuilder_AddFan(MESH_BUILDER *builder, bool transparent);

// Applies invisible z offset to all vertices that helps with the z-fighting.
void MeshBuilder_AdjustDepth(MESH_BUILDER *builder, float depth);

// Finalize all pending vertices and faces into the OUTPUT_MESH and seal it.
// Returns the sealed mesh; builder must still be destroyed via
// MeshBuilder_Destroy().
OUTPUT_MESH *MeshBuilder_Seal(MESH_BUILDER *builder);

// Utility method to add a quad representing a room sprite.
void MeshBuilder_AddRoomSprite(
    MESH_BUILDER *builder, const ROOM_SPRITE *room_sprite, const ROOM *room,
    float depth_adjust);
