#include "game/output/mesh_batcher/mesh_builder.h"

#include "debug.h"
#include "game/output/mesh_batcher/mesh.h"
#include "game/output/shader.h"
#include "game/output/textures.h"
#include "game/output/vertex_range.h"
#include "memory.h"

struct MESH_BUILDER {
    size_t pending_vertex_count;
    OUTPUT_MESH *mesh;
    VECTOR *indices;
};

static const size_t m_Size3 = 3;
static const size_t m_Size4 = 6;
static const int32_t m_Indices3[] = { 0, 2, 1 };
static const int32_t m_Indices4[] = { 0, 2, 1, 0, 3, 2 };

static void M_EnsureMesh(MESH_BUILDER *const builder)
{
    if (builder->mesh == nullptr) {
        builder->mesh = Output_Mesh_Create();
        builder->pending_vertex_count = 0;
    }
}

MESH_BUILDER *MeshBuilder_Create(void)
{
    MESH_BUILDER *const builder = Memory_Alloc(sizeof(*builder));
    builder->indices = Vector_Create(sizeof(int32_t));
    return builder;
}

void MeshBuilder_Destroy(MESH_BUILDER *const builder)
{
    ASSERT(builder != nullptr);
    if (builder->mesh != nullptr) {
        Output_Mesh_Destroy(builder->mesh);
        builder->mesh = nullptr;
    }
    if (builder->indices != nullptr) {
        Vector_Free(builder->indices);
        builder->indices = nullptr;
    }

    Memory_Free(builder);
}

void MeshBuilder_AddVertex(
    MESH_BUILDER *const builder, const OUTPUT_MESH_VERTEX *const vertex)
{
    ASSERT(builder != nullptr);
    M_EnsureMesh(builder);
    ASSERT(builder->mesh != nullptr);
    ASSERT(!builder->mesh->sealed);
    if (!(vertex->flags & VERT_FLAT_SHADED)
        && Output_Textures_IsObjectTextureAnimated(vertex->uvw_idx / 4)) {
        Vector_Add(
            builder->mesh->animated_vertices,
            &(OUTPUT_VERTEX_RANGE) {
                .vertex_start = builder->mesh->vertices->count,
                .vertex_count = 1,
            });
    }
    Vector_Add(builder->mesh->vertices, vertex);
    builder->pending_vertex_count++;
}

void MeshBuilder_AddFace(
    MESH_BUILDER *const builder, const bool transparent, const int32_t *indices,
    const size_t idx_count)
{
    ASSERT(builder != nullptr);
    M_EnsureMesh(builder);
    ASSERT(builder->mesh != nullptr);
    ASSERT(!builder->mesh->sealed);

    const size_t vtx_count = builder->pending_vertex_count;
    const size_t start = builder->mesh->vertices->count - vtx_count;
    const OUTPUT_MESH_VERTEX *const vbuf =
        Vector_GetData(builder->mesh->vertices);
    XYZ_F centroid = { 0.0f, 0.0f, 0.0f };
    for (size_t i = 0; i < vtx_count; i++) {
        centroid.x += vbuf[start + i].pos.x;
        centroid.y += vbuf[start + i].pos.y;
        centroid.z += vbuf[start + i].pos.z;
    }
    centroid.x /= (float)vtx_count;
    centroid.y /= (float)vtx_count;
    centroid.z /= (float)vtx_count;
    if (transparent) {
        OUTPUT_MESH_FACE face = {
            .vertex_count = idx_count,
            .mesh_centroid = centroid,
        };
        face.vertex_indices = Memory_ArenaAlloc(
            &builder->mesh->allocator, sizeof(int32_t) * idx_count);
        for (size_t i = 0; i < idx_count; i++) {
            face.vertex_indices[i] = start + indices[i];
        }
        Vector_Add(builder->mesh->transparent_faces, &face);
    } else {
        for (size_t i = 0; i < idx_count; i++) {
            Vector_Add(
                builder->mesh->opaque_vertex_indices,
                &(uint32_t) { start + indices[i] });
        }
    }
    builder->pending_vertex_count = 0;
}

void MeshBuilder_AddFace3(MESH_BUILDER *const builder, const bool transparent)
{
    MeshBuilder_AddFace(builder, transparent, m_Indices3, m_Size3);
}

void MeshBuilder_AddFace4(MESH_BUILDER *const builder, const bool transparent)
{
    MeshBuilder_AddFace(builder, transparent, m_Indices4, m_Size4);
}

void MeshBuilder_AddFan(MESH_BUILDER *const builder, const bool transparent)
{
    ASSERT(builder != nullptr);
    M_EnsureMesh(builder);
    const size_t vtx_count = builder->pending_vertex_count;
    ASSERT(vtx_count >= 3);
    const size_t segment_count = vtx_count - 2;
    const size_t idx_count = segment_count * 3;
    for (size_t i = 0; i < segment_count; i++) {
        Vector_Add(builder->indices, &(int32_t) { 0 });
        Vector_Add(builder->indices, &(int32_t) { i + 2 });
        Vector_Add(builder->indices, &(int32_t) { i + 1 });
    }
    MeshBuilder_AddFace(
        builder, transparent, Vector_GetData(builder->indices), idx_count);
    Vector_Clear(builder->indices);
}

void MeshBuilder_AddRoomSprite(
    MESH_BUILDER *const builder, const ROOM_SPRITE *const room_sprite,
    const ROOM *const room, const float depth_adjust)
{
    const int16_t texture_idx = room_sprite->texture;
    const SPRITE_TEXTURE *const sprite = Output_GetSpriteTexture(texture_idx);
    const ROOM_VERTEX *const room_vert =
        &room->mesh.vertices[room_sprite->vertex];
    const XYZ_16 *pos = &room_vert->pos;
    const struct {
        float x, y;
    } normal[4] = {
        { .x = sprite->x0, .y = sprite->y0 },
        { .x = sprite->x1, .y = sprite->y0 },
        { .x = sprite->x1, .y = sprite->y1 },
        { .x = sprite->x0, .y = sprite->y1 },
    };
    for (int32_t j = 0; j < 4; j++) {
        const OUTPUT_MESH_VERTEX vertex = {
            .pos = { .x = pos->x, .y = pos->y, .z = pos->z, .w = depth_adjust },
            .normal = { .x = normal[j].x, .y = normal[j].y, .z = 0.0f },
            .flags = Output_Textures_GetSpriteTextureFlags(texture_idx),
            .color = { 255, 255, 255, 255 },
            .uvw_idx = Output_Textures_GetSpriteUVWIndex(texture_idx, j),
            .shade = room_vert->light_adder,
            .trapezoid_ratio = { 1.0f, 1.0f },
        };
        MeshBuilder_AddVertex(builder, &vertex);
    }
    MeshBuilder_AddFace4(builder, true);
}

void MeshBuilder_AdjustDepth(MESH_BUILDER *const builder, const float depth)
{
    if (builder->mesh == nullptr || builder->mesh->vertices == nullptr) {
        return;
    }
    OUTPUT_MESH_VERTEX *const vbuf = Vector_GetData(builder->mesh->vertices);
    for (int32_t i = 0; i < builder->mesh->vertices->count; i++) {
        vbuf[i].pos.w = depth;
    }
}

OUTPUT_MESH *MeshBuilder_Seal(MESH_BUILDER *const builder)
{
    ASSERT(builder != nullptr);
    if (builder->mesh == nullptr) {
        return nullptr;
    }
    OUTPUT_MESH *const mesh = builder->mesh;
    Output_GlueVertexRanges(mesh->animated_vertices);
    mesh->sealed = 1;
    builder->mesh = nullptr;
    return mesh;
}
