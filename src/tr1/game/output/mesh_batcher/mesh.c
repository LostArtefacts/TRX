#include "game/output/mesh_batcher/mesh.h"

#include "game/output.h"
#include "game/output/textures.h"
#include "game/output/utils.h"
#include "game/output/vertex_range.h"

#include <libtrx/debug.h>
#include <libtrx/game/level/const.h>
#include <libtrx/memory.h>

static XYZ_F M_GetWorldCentroid(
    const OUTPUT_MESH *mesh, int32_t vertex_start, int32_t vertex_count);
static void M_AddFaceCommon(
    OUTPUT_MESH *mesh, int32_t vertex_start, int32_t vertex_count,
    bool is_transparent);
static void M_AddRoomVert(
    OUTPUT_MESH *mesh, const ROOM_VERTEX *room_vert, int32_t uvw_idx,
    const TEXTURE_ZW_F *texture_ratio);
static void M_AddObjectVert(
    OUTPUT_MESH *mesh, const XYZ_16 *pos, int32_t uvw_idx,
    const TEXTURE_ZW_F *texture_ratio, uint16_t flags, XYZ_16 normal,
    int16_t palette_idx);

static XYZ_F M_GetMeshCentroid(
    const OUTPUT_MESH *const mesh, const int32_t vertex_start,
    const int32_t vertex_count)
{
    const OUTPUT_MESH_VERTEX *const vbuf = Vector_GetData(mesh->vertices);
    XYZ_F wc = { 0.0f, 0.0f, 0.0f };
    for (int32_t i = 0; i < vertex_count; i++) {
        wc.x += vbuf[vertex_start + i].pos.x;
        wc.y += vbuf[vertex_start + i].pos.y;
        wc.z += vbuf[vertex_start + i].pos.z;
    }
    wc.x /= vertex_count;
    wc.y /= vertex_count;
    wc.z /= vertex_count;
    return wc;
}

static void M_AddFaceCommon(
    OUTPUT_MESH *const mesh, const int32_t vertex_start,
    const int32_t vertex_count, const bool is_transparent)
{
    XYZ_F mesh_centroid = M_GetMeshCentroid(mesh, vertex_start, vertex_count);
    const OUTPUT_MESH_FACE face = {
        .vertex_start = vertex_start,
        .vertex_count = vertex_count,
        .mesh_centroid = mesh_centroid,
    };
    if (is_transparent) {
        Vector_Add(mesh->transparent_faces, &face);
    } else {
        for (int32_t i = 0; i < vertex_count; i++) {
            Vector_Add(
                mesh->opaque_vertex_indices, &(int32_t) { vertex_start + i });
        }
    }
}

static void M_AddRoomVert(
    OUTPUT_MESH *const mesh, const ROOM_VERTEX *const room_vert,
    const int32_t uvw_idx, const TEXTURE_ZW_F *const texture_ratio)
{
    if (Output_Textures_IsObjectTextureAnimated(uvw_idx / 4)) {
        Vector_Add(
            mesh->animated_vertices,
            &(OUTPUT_VERTEX_RANGE) {
                .vertex_start = mesh->vertices->count,
                .vertex_count = 1,
            });
    }
    OUTPUT_MESH_VERTEX vertex = {
        .pos = {
            .x = room_vert->pos.x,
            .y = room_vert->pos.y,
            .z = room_vert->pos.z,
        },
        .flags = room_vert->flags & NO_VERT_MOVE ? VERT_NO_CAUSTICS : 0,
        .uvw_idx = uvw_idx,
        .shade = room_vert->light_adder,
        .color = { 255, 255, 255, 255 },
        .trapezoid_ratio = {
            [0] = texture_ratio != nullptr ? texture_ratio->z : 1.0f,
            [1] = texture_ratio != nullptr ? texture_ratio->w : 1.0f,
        },
    };
    Vector_Add(mesh->vertices, &vertex);
}

static void M_AddObjectVert(
    OUTPUT_MESH *const mesh, const XYZ_16 *const pos, const int32_t uvw_idx,
    const TEXTURE_ZW_F *const texture_ratio, const uint16_t flags,
    const XYZ_16 normal, const int16_t palette_idx)
{
    if (!(flags & VERT_FLAT_SHADED)
        && Output_Textures_IsObjectTextureAnimated(uvw_idx / 4)) {
        Vector_Add(
            mesh->animated_vertices,
            &(OUTPUT_VERTEX_RANGE) {
                .vertex_start = mesh->vertices->count,
                .vertex_count = 1,
            });
    }
    OUTPUT_MESH_VERTEX vertex = {
        .pos = {
            .x = pos->x,
            .y = pos->y,
            .z = pos->z,
        },
        .normal = {
            .x = normal.x,
            .y = normal.y,
            .z = normal.z,
        },
        .flags = flags,
        .uvw_idx = uvw_idx,
        .shade = SHADE_NEUTRAL,
        .color = (flags & VERT_FLAT_SHADED) ?
            Output_RGB2RGBA(Output_GetPaletteColor8(palette_idx))
            : (RGBA_8888) { 255, 255, 255, 255 },
        .trapezoid_ratio = {
            [0] = texture_ratio != nullptr ? texture_ratio->z : 1.0f,
            [1] = texture_ratio != nullptr ? texture_ratio->w : 1.0f,
        },
    };
    Vector_Add(mesh->vertices, &vertex);
}

OUTPUT_MESH *Output_Mesh_Create(void)
{
    OUTPUT_MESH *const mesh = Memory_Alloc(sizeof(OUTPUT_MESH));
    mesh->vertices = Vector_Create(sizeof(OUTPUT_MESH_VERTEX));
    mesh->animated_vertices = Vector_Create(sizeof(OUTPUT_VERTEX_RANGE));
    mesh->transparent_faces = Vector_Create(sizeof(OUTPUT_MESH_FACE));
    mesh->opaque_vertex_indices = Vector_Create(sizeof(int32_t));
    mesh->sealed = false;
    return mesh;
}

void Output_Mesh_AddRoomFace4(
    OUTPUT_MESH *const mesh, const FACE4 *const face, const ROOM *const room)
{
    ASSERT(!mesh->sealed);
    const int32_t vertex_start = mesh->vertices->count;
    for (int32_t i = 0; i < OUTPUT_QUAD_VERTICES; i++) {
        const int32_t j = OUTPUT_QUAD_TO_FAN(i);
        const int32_t uvw_idx = face->texture_idx * 4 + j;
        const ROOM_VERTEX *const room_vert =
            &room->mesh.vertices[face->vertices[j]];
        M_AddRoomVert(mesh, room_vert, uvw_idx, &face->texture_zw[j]);
    }
    const int32_t vertex_count = mesh->vertices->count - vertex_start;
    M_AddFaceCommon(
        mesh, vertex_start, vertex_count,
        Output_Textures_IsObjectTextureTransparent(face->texture_idx));
}

void Output_Mesh_AddRoomFace3(
    OUTPUT_MESH *const mesh, const FACE3 *const face, const ROOM *const room)
{
    ASSERT(!mesh->sealed);
    const int32_t vertex_start = mesh->vertices->count;
    for (int32_t i = 0; i < OUTPUT_TRI_VERTICES; i++) {
        const int32_t j = OUTPUT_TRI_TO_FAN(i);
        const int32_t uvw_idx = face->texture_idx * 4 + j;
        const ROOM_VERTEX *const room_vert =
            &room->mesh.vertices[face->vertices[j]];
        M_AddRoomVert(mesh, room_vert, uvw_idx, nullptr);
    }
    const int32_t vertex_count = mesh->vertices->count - vertex_start;
    M_AddFaceCommon(
        mesh, vertex_start, vertex_count,
        Output_Textures_IsObjectTextureTransparent(face->texture_idx));
}

void Output_Mesh_AddRoomSprite(
    OUTPUT_MESH *const mesh, const ROOM_SPRITE *const room_sprite,
    const ROOM *const room)
{
    ASSERT(!mesh->sealed);
    const int32_t vertex_start = mesh->vertices->count;
    struct {
        struct {
            float x, y;
        } displacement;
    } quad[4];
    const SPRITE_TEXTURE *const sprite =
        Output_GetSpriteTexture(room_sprite->texture);
    quad[0].displacement.x = sprite->x0;
    quad[0].displacement.y = sprite->y0;
    quad[1].displacement.x = sprite->x1;
    quad[1].displacement.y = sprite->y0;
    quad[2].displacement.x = sprite->x1;
    quad[2].displacement.y = sprite->y1;
    quad[3].displacement.x = sprite->x0;
    quad[3].displacement.y = sprite->y1;
    for (int32_t i = 0; i < OUTPUT_QUAD_VERTICES; i++) {
        const int32_t j = OUTPUT_QUAD_TO_FAN(i);
        const ROOM_VERTEX *const room_vert =
            &room->mesh.vertices[room_sprite->vertex];
        if (Output_Textures_IsSpriteTextureAnimated(room_sprite->texture)) {
            Vector_Add(
                mesh->animated_vertices,
                &(OUTPUT_VERTEX_RANGE) {
                    .vertex_start = mesh->vertices->count,
                    .vertex_count = 1,
                });
        }
        OUTPUT_MESH_VERTEX vertex = {
            .pos = {
                .x = room_vert->pos.x,
                .y = room_vert->pos.y,
                .z = room_vert->pos.z,
            },
            .normal = {
                .x = quad[j].displacement.x,
                .y = quad[j].displacement.y,
                .z = 0.0f,
            },
            .flags = VERT_BILLBOARD,
            .color = { 255, 255, 255, 255 },
            .uvw_idx = Output_Textures_GetSpritesUVWsBase()
                + room_sprite->texture * 4 + j,
            .shade = room_vert->light_adder,
            .trapezoid_ratio = { 1.0f, 1.0f },
        };
        Vector_Add(mesh->vertices, &vertex);
    }
    const int32_t vertex_count = mesh->vertices->count - vertex_start;
    M_AddFaceCommon(mesh, vertex_start, vertex_count, true);
}

void Output_Mesh_AddObjectFace4(
    OUTPUT_MESH *const mesh, const OBJECT_MESH *const obj_mesh,
    const FACE4 *const face, const uint16_t flags)
{
    ASSERT(!mesh->sealed);
    const int32_t vertex_start = mesh->vertices->count;
    for (int32_t i = 0; i < OUTPUT_QUAD_VERTICES; i++) {
        const int32_t j = OUTPUT_QUAD_TO_FAN(i);
        const int32_t uvw_idx = face->texture_idx * 4 + j;
        const XYZ_16 normal = obj_mesh->lighting.normals[face->vertices[j]];
        const XYZ_16 *const pos = &obj_mesh->vertices[face->vertices[j]];
        M_AddObjectVert(
            mesh, pos, uvw_idx, &face->texture_zw[j], flags, normal,
            face->palette_idx);
    }
    const int32_t vertex_count = mesh->vertices->count - vertex_start;
    M_AddFaceCommon(
        mesh, vertex_start, vertex_count,
        Output_Textures_IsObjectTextureTransparent(face->texture_idx));
}

void Output_Mesh_AddObjectFace3(
    OUTPUT_MESH *const mesh, const OBJECT_MESH *const obj_mesh,
    const FACE3 *const face, const uint16_t flags)
{
    ASSERT(!mesh->sealed);
    const int32_t vertex_start = mesh->vertices->count;
    for (int32_t i = 0; i < OUTPUT_TRI_VERTICES; i++) {
        const int32_t j = OUTPUT_TRI_TO_FAN(i);
        const int32_t uvw_idx = face->texture_idx * 4 + j;
        const XYZ_16 normal = obj_mesh->lighting.normals[face->vertices[j]];
        const XYZ_16 *const pos = &obj_mesh->vertices[face->vertices[j]];
        M_AddObjectVert(
            mesh, pos, uvw_idx, nullptr, flags, normal, face->palette_idx);
    }
    const int32_t vertex_count = mesh->vertices->count - vertex_start;
    M_AddFaceCommon(
        mesh, vertex_start, vertex_count,
        Output_Textures_IsObjectTextureTransparent(face->texture_idx));
}

void Output_Mesh_Seal(OUTPUT_MESH *const mesh)
{
    Output_GlueVertexRanges(mesh->animated_vertices);
    mesh->sealed = true;
}

void Output_Mesh_Destroy(OUTPUT_MESH *const mesh)
{
    if (mesh->animated_vertices != nullptr) {
        Vector_Free(mesh->animated_vertices);
    }
    Vector_Free(mesh->vertices);
    if (mesh->transparent_faces != nullptr) {
        Vector_Free(mesh->transparent_faces);
    }
    if (mesh->opaque_vertex_indices != nullptr) {
        Vector_Free(mesh->opaque_vertex_indices);
    }
    Memory_Free(mesh);
}
