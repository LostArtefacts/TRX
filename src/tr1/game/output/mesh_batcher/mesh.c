#include "game/output/mesh_batcher/mesh.h"

#include "game/output.h"
#include "game/output/textures.h"
#include "game/output/utils.h"
#include "game/output/vertex_range.h"

#include <libtrx/debug.h>
#include <libtrx/game/level/const.h>
#include <libtrx/memory.h>

static void M_AddRoomVert(
    OUTPUT_MESH *b, const ROOM_VERTEX *room_vert, int32_t uvw_idx,
    const TEXTURE_ZW_F *texture_ratio);
static void M_AddObjectVert(
    OUTPUT_MESH *b, const XYZ_16 *pos, int32_t uvw_idx,
    const TEXTURE_ZW_F *texture_ratio, uint16_t flags, XYZ_16 normal,
    int16_t palette_idx);

static void M_AddRoomVert(
    OUTPUT_MESH *const b, const ROOM_VERTEX *const room_vert,
    const int32_t uvw_idx, const TEXTURE_ZW_F *const texture_ratio)
{
    if (Output_Textures_IsObjectTextureAnimated(uvw_idx / 4)) {
        Vector_Add(
            b->animated_vertices,
            &(OUTPUT_VERTEX_RANGE) {
                .vertex_start = b->vertices->count,
                .vertex_count = 1,
            });
    }
    OUTPUT_MESH_VERTEX vertex = {
        .pos = (XYZ_F) {
            .x = room_vert->pos.x,
            .y = room_vert->pos.y,
            .z = room_vert->pos.z,
        },
        .flags = room_vert->flags & NO_VERT_MOVE ? VERT_NO_CAUSTICS : 0,
        .uvw_idx = uvw_idx,
        .shade = room_vert->light_adder,
        .color = (RGBA_8888) { 255, 255, 255, 255 },
        .trapezoid_ratio = {
            [0] = texture_ratio != nullptr ? texture_ratio->z : 1.0f,
            [1] = texture_ratio != nullptr ? texture_ratio->w : 1.0f,
        },
    };
    Vector_Add(b->vertices, &vertex);
}

static void M_AddObjectVert(
    OUTPUT_MESH *const b, const XYZ_16 *const pos, const int32_t uvw_idx,
    const TEXTURE_ZW_F *const texture_ratio, const uint16_t flags,
    const XYZ_16 normal, const int16_t palette_idx)
{
    if (!(flags & VERT_FLAT_SHADED)
        && Output_Textures_IsObjectTextureAnimated(uvw_idx / 4)) {
        Vector_Add(
            b->animated_vertices,
            &(OUTPUT_VERTEX_RANGE) {
                .vertex_start = b->vertices->count,
                .vertex_count = 1,
            });
    }
    OUTPUT_MESH_VERTEX vertex = {
        .pos = (XYZ_F) {
            .x = pos->x,
            .y = pos->y,
            .z = pos->z,
        },
        .normal = (XYZ_F) {
            .x = normal.x,
            .y = normal.y,
            .z = normal.z,
        },
        .flags = flags,
        .uvw_idx = uvw_idx,
        .shade = SHADE_NEUTRAL,
        .color = (flags & VERT_FLAT_SHADED) ?
            Output_RGB2RGBA(Output_GetPaletteColor8(palette_idx))
            :  (RGBA_8888) { 255, 255, 255, 255 },
        .trapezoid_ratio = {
            [0] = texture_ratio != nullptr ? texture_ratio->z : 1.0f,
            [1] = texture_ratio != nullptr ? texture_ratio->w : 1.0f,
        },
    };
    Vector_Add(b->vertices, &vertex);
}

OUTPUT_MESH *Output_Mesh_Create(void)
{
    OUTPUT_MESH *const b = Memory_Alloc(sizeof(OUTPUT_MESH));
    b->vertices = Vector_Create(sizeof(OUTPUT_MESH_VERTEX));
    b->animated_vertices = Vector_Create(sizeof(OUTPUT_VERTEX_RANGE));
    return b;
}

void Output_Mesh_AddRoomFace4(
    OUTPUT_MESH *const b, const FACE4 *const face,
    const ROOM_VERTEX *const verts)
{
    ASSERT(!b->sealed);
    for (int32_t i = 0; i < OUTPUT_QUAD_VERTICES; i++) {
        const int32_t j = OUTPUT_QUAD_TO_FAN(i);
        const int32_t uvw_idx = face->texture_idx * 4 + j;
        const ROOM_VERTEX *const room_vert = &verts[face->vertices[j]];
        M_AddRoomVert(b, room_vert, uvw_idx, &face->texture_zw[j]);
    }
}

void Output_Mesh_AddRoomFace3(
    OUTPUT_MESH *const b, const FACE3 *const face,
    const ROOM_VERTEX *const verts)
{
    ASSERT(!b->sealed);
    for (int32_t i = 0; i < OUTPUT_TRI_VERTICES; i++) {
        const int32_t j = OUTPUT_TRI_TO_FAN(i);
        const int32_t uvw_idx = face->texture_idx * 4 + j;
        const ROOM_VERTEX *const room_vert = &verts[face->vertices[j]];
        M_AddRoomVert(b, room_vert, uvw_idx, nullptr);
    }
}

void Output_Mesh_AddRoomSprite(
    OUTPUT_MESH *const b, const ROOM_SPRITE *const room_sprite,
    const ROOM_VERTEX *const verts)
{
    ASSERT(!b->sealed);
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
        const ROOM_VERTEX *const room_vert = &verts[room_sprite->vertex];
        if (Output_Textures_IsSpriteTextureAnimated(room_sprite->texture)) {
            Vector_Add(
                b->animated_vertices,
                &(OUTPUT_VERTEX_RANGE) {
                    .vertex_start = b->vertices->count,
                    .vertex_count = 1,
                });
        }
        OUTPUT_MESH_VERTEX vertex = {
            .pos =
                (XYZ_F) {
                    .x = room_vert->pos.x,
                    .y = room_vert->pos.y,
                    .z = room_vert->pos.z,
                },
            .normal =
                (XYZ_F) {
                    .x = quad[j].displacement.x,
                    .y = quad[j].displacement.y,
                    .z = 0.0f,
                },
            .flags = VERT_BILLBOARD,
            .color = (RGBA_8888) { 255, 255, 255, 255 },
            .uvw_idx = Output_Textures_GetSpritesUVWsBase()
                + room_sprite->texture * 4 + j,
            .shade = room_vert->light_adder,
            .trapezoid_ratio = { 1.0f, 1.0f },
        };
        Vector_Add(b->vertices, &vertex);
    }
}

void Output_Mesh_AddObjectFace4(
    OUTPUT_MESH *const b, const OBJECT_MESH *const obj_mesh,
    const FACE4 *const face, const uint16_t flags)
{
    ASSERT(!b->sealed);
    for (int32_t i = 0; i < OUTPUT_QUAD_VERTICES; i++) {
        const int32_t j = OUTPUT_QUAD_TO_FAN(i);
        const int32_t uvw_idx = face->texture_idx * 4 + j;
        const XYZ_16 normal = obj_mesh->lighting.normals[face->vertices[j]];
        const XYZ_16 *const pos = &obj_mesh->vertices[face->vertices[j]];
        M_AddObjectVert(
            b, pos, uvw_idx, &face->texture_zw[j], flags, normal,
            face->palette_idx);
    }
}

void Output_Mesh_AddObjectFace3(
    OUTPUT_MESH *const b, const OBJECT_MESH *const obj_mesh,
    const FACE3 *const face, const uint16_t flags)
{
    ASSERT(!b->sealed);
    for (int32_t i = 0; i < OUTPUT_TRI_VERTICES; i++) {
        const int32_t j = OUTPUT_TRI_TO_FAN(i);
        const int32_t uvw_idx = face->texture_idx * 4 + j;
        const XYZ_16 normal = obj_mesh->lighting.normals[face->vertices[j]];
        const XYZ_16 *const pos = &obj_mesh->vertices[face->vertices[j]];
        M_AddObjectVert(
            b, pos, uvw_idx, nullptr, flags, normal, face->palette_idx);
    }
}

void Output_Mesh_Seal(OUTPUT_MESH *const b)
{
    Output_GlueVertexRanges(b->animated_vertices);
    b->sealed = true;
}

void Output_Mesh_Destroy(OUTPUT_MESH *const b)
{
    if (b->animated_vertices != nullptr) {
        Vector_Free(b->animated_vertices);
    }
    Vector_Free(b->vertices);
    Memory_Free(b);
}
