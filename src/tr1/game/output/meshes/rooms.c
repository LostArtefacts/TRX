#include "game/output/meshes/rooms.h"

#include "game/output.h"
#include "game/output/meshes/common.h"
#include "game/output/textures.h"
#include "game/output/utils.h"
#include "game/output/vertex_range.h"
#include "game/random.h"
#include "game/room.h"

#include <libtrx/gfx/gl/utils.h>
#include <libtrx/memory.h>
#include <libtrx/vector.h>

#pragma pack(push, 1)
typedef struct {
    // attribute 0
    XYZ_F pos;
    // attribute 5
    OUTPUT_USHORT flags;
} M_MESH_VERTEX;

// attribute 7
typedef OUTPUT_USHORT M_MESH_SHADE;
#pragma pack(pop)

typedef struct {
    int32_t vertex_start;
    int32_t vertex_count;
    int16_t *caustics;
    VECTOR *animated_vertices;
} M_BATCH;

static struct {
    size_t vertex_count;
    GLuint vao;
    GLuint geom_vbo;
    GLuint tex_vbo;
    GLuint shade_vbo;
    M_MESH_VERTEX *geom_vbo_data;
    OUTPUT_MESH_TEXTURE *tex_vbo_data;
    M_MESH_SHADE *shade_vbo_data;
    size_t batch_count;
    M_BATCH *batches;
} m_Priv;

static int32_t m_ShadeTable[WIBBLE_SIZE] = {};
static int32_t m_CausticsTable[WIBBLE_SIZE] = {};
static OUTPUT_SHADER *m_Shader = nullptr;

static M_MESH_SHADE M_ShadeCaustics(M_MESH_SHADE source, uint8_t caustic)
{
    if (Output_GetWaterEffect()) {
        source +=
            m_ShadeTable[((uint8_t)Output_GetTime() + caustic) % WIBBLE_SIZE];
        CLAMP(source, 0, SHADE_MAX);
    } else {
        CLAMPG(source, SHADE_MAX);
    }
    return source;
}

static M_BATCH *M_GetBatch(const ROOM *const room)
{
    // Room data gets swapped when flipping, but the VBOs do not. So a room 2
    // that gets flipped to room 17 ends up getting the data from room 2,
    // whereas the VBO needs to take data from room 17.
    const int16_t room_num =
        Room_GetFlipStatus() && room->flipped_room != NO_ROOM_NEG
        ? room->flipped_room
        : Room_GetNumber(room);
    return &m_Priv.batches[room_num];
}

static void M_FillVertex(
    M_MESH_VERTEX *const out_vertex, const XYZ_16 pos, const uint16_t flags)
{
    out_vertex->pos = (XYZ_F) { .x = pos.x, .y = pos.y, .z = pos.z };
    out_vertex->flags = flags & NO_VERT_MOVE ? VERT_NO_CAUSTICS : 0;
}

static void M_FillTexture(
    OUTPUT_MESH_TEXTURE *const out_texture, const int32_t uvw_idx,
    const float trapezoid_ratio_x, const float trapezoid_ratio_y)
{
    out_texture->uvw = Output_Textures_GetUVW(uvw_idx);
    out_texture->texture_size = Output_Textures_GetAtlasSize(uvw_idx / 4);
    out_texture->trapezoid_ratio[0] = trapezoid_ratio_x;
    out_texture->trapezoid_ratio[1] = trapezoid_ratio_y;
}

static void M_FillRoomFace4(
    const ROOM *const room, M_MESH_VERTEX **out_vertex,
    OUTPUT_MESH_TEXTURE **out_texture, const FACE4 *const face)
{
    for (int32_t i = 0; i < OUTPUT_QUAD_VERTICES; i++) {
        const int32_t j = OUTPUT_QUAD_TO_FAN(i);
        const int32_t uvw_idx = face->texture_idx * 4 + j;
        const ROOM_VERTEX *const room_vertex =
            &room->mesh.vertices[face->vertices[j]];
        M_FillVertex(*out_vertex, room_vertex->pos, room_vertex->flags);
        M_FillTexture(
            *out_texture, uvw_idx, face->texture_zw[j].z,
            face->texture_zw[j].w);
        (*out_vertex)++;
        (*out_texture)++;
    }
}

static void M_FillRoomFace3(
    const ROOM *const room, M_MESH_VERTEX **out_vertex,
    OUTPUT_MESH_TEXTURE **out_texture, const FACE3 *const face)
{
    for (int32_t i = 0; i < OUTPUT_TRI_VERTICES; i++) {
        const int32_t j = OUTPUT_TRI_TO_FAN(i);
        const int32_t uvw_idx = face->texture_idx * 4 + j;
        const ROOM_VERTEX *const room_vertex =
            &room->mesh.vertices[face->vertices[j]];
        M_FillVertex(*out_vertex, room_vertex->pos, room_vertex->flags);
        M_FillTexture(*out_texture, uvw_idx, 1.0f, 1.0f);
        (*out_vertex)++;
        (*out_texture)++;
    }
}

static void M_UpdateRoomGeometry(const ROOM *const room)
{
    M_BATCH *const batch = &m_Priv.batches[Room_GetNumber(room)];
    M_MESH_VERTEX *out_vertex = &m_Priv.geom_vbo_data[batch->vertex_start];
    OUTPUT_MESH_TEXTURE *out_texture =
        &m_Priv.tex_vbo_data[batch->vertex_start];
    for (int32_t i = 0; i < room->mesh.num_face4s; i++) {
        M_FillRoomFace4(room, &out_vertex, &out_texture, &room->mesh.face4s[i]);
    }
    for (int32_t i = 0; i < room->mesh.num_face3s; i++) {
        M_FillRoomFace3(room, &out_vertex, &out_texture, &room->mesh.face3s[i]);
    }
}

static void M_UpdateRoomShades(const ROOM *const room)
{
    M_BATCH *const batch = M_GetBatch(room);
    M_MESH_SHADE *out_shade = &m_Priv.shade_vbo_data[batch->vertex_start];
    for (int32_t i = 0; i < room->mesh.num_face4s; i++) {
        const FACE4 *const face = &room->mesh.face4s[i];
        for (int32_t j = 0; j < OUTPUT_QUAD_VERTICES; j++) {
            const int32_t k = OUTPUT_QUAD_TO_FAN(j);
            *out_shade = room->mesh.vertices[face->vertices[k]].light_adder;
            *out_shade =
                M_ShadeCaustics(*out_shade, batch->caustics[face->vertices[k]]);
            *out_shade++;
        }
    }
    for (int32_t i = 0; i < room->mesh.num_face3s; i++) {
        const FACE3 *const face = &room->mesh.face3s[i];
        for (int32_t j = 0; j < OUTPUT_TRI_VERTICES; j++) {
            const int32_t k = OUTPUT_TRI_TO_FAN(j);
            *out_shade = room->mesh.vertices[face->vertices[k]].light_adder;
            *out_shade =
                M_ShadeCaustics(*out_shade, batch->caustics[face->vertices[k]]);
            out_shade++;
        }
    }
}

static void M_UpdateVertices(void)
{
    for (int32_t i = 0; i < Room_GetCount(); i++) {
        const ROOM *const room = Room_Get(i);
        M_UpdateRoomGeometry(room);
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_Priv.geom_vbo);
    GFX_TRACK_DATA(
        glBufferData, GL_ARRAY_BUFFER,
        m_Priv.vertex_count * sizeof(M_MESH_VERTEX), m_Priv.geom_vbo_data,
        GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, m_Priv.tex_vbo);
    GFX_TRACK_DATA(
        glBufferData, GL_ARRAY_BUFFER,
        m_Priv.vertex_count * sizeof(OUTPUT_MESH_TEXTURE), m_Priv.tex_vbo_data,
        GL_DYNAMIC_DRAW); // allow animating textures

    glBindBuffer(GL_ARRAY_BUFFER, m_Priv.shade_vbo);
    GFX_TRACK_DATA(
        glBufferData, GL_ARRAY_BUFFER,
        m_Priv.vertex_count * sizeof(M_MESH_SHADE), m_Priv.shade_vbo_data,
        GL_DYNAMIC_DRAW); // shades are always dynamic
}

static void M_PrepareBuffers(void)
{
    m_Priv.batch_count = Room_GetCount();
    Memory_FreePointer(&m_Priv.batches);
    m_Priv.batches = Memory_Alloc(sizeof(M_BATCH) * m_Priv.batch_count);

    int32_t last_vertex = 0;
    for (int32_t i = 0; i < Room_GetCount(); i++) {
        const ROOM *const room = Room_Get(i);
        M_BATCH *const batch = &m_Priv.batches[i];
        batch->caustics =
            Memory_Alloc(room->mesh.num_vertices * sizeof(int16_t));
        batch->vertex_start = last_vertex;
        batch->vertex_count = 0;
        batch->vertex_count += room->mesh.num_face4s * OUTPUT_QUAD_VERTICES;
        batch->vertex_count += room->mesh.num_face3s * OUTPUT_TRI_VERTICES;
        last_vertex += batch->vertex_count;

        for (int32_t j = 0; j < room->mesh.num_vertices; j++) {
            batch->caustics[j] =
                m_CausticsTable[(room->mesh.num_vertices - j) % WIBBLE_SIZE];
        }
    }
    m_Priv.vertex_count = last_vertex;

    int32_t current_vertex = 0;
    for (int32_t i = 0; i < Room_GetCount(); i++) {
        const ROOM *const room = Room_Get(i);
        M_BATCH *const batch = M_GetBatch(room);
        batch->animated_vertices = Vector_Create(sizeof(OUTPUT_VERTEX_RANGE));
        for (int32_t j = 0; j < room->mesh.num_face4s; j++) {
            const FACE4 *const face = &room->mesh.face4s[j];
            if (Output_Textures_IsObjectTextureAnimated(face->texture_idx)) {
                Vector_Add(
                    batch->animated_vertices,
                    &(OUTPUT_VERTEX_RANGE) {
                        .vertex_start = current_vertex,
                        .vertex_count = OUTPUT_QUAD_VERTICES,
                    });
            }
            current_vertex += OUTPUT_QUAD_VERTICES;
        }
        for (int32_t j = 0; j < room->mesh.num_face3s; j++) {
            const FACE3 *const face = &room->mesh.face3s[j];
            if (Output_Textures_IsObjectTextureAnimated(face->texture_idx)) {
                Vector_Add(
                    batch->animated_vertices,
                    &(OUTPUT_VERTEX_RANGE) {
                        .vertex_start = current_vertex,
                        .vertex_count = OUTPUT_TRI_VERTICES,
                    });
            }
            current_vertex += OUTPUT_TRI_VERTICES;
        }
        Output_GlueVertexRanges(batch->animated_vertices);
    }

    m_Priv.geom_vbo_data =
        Memory_Alloc(m_Priv.vertex_count * sizeof(M_MESH_VERTEX));
    m_Priv.tex_vbo_data =
        Memory_Alloc(m_Priv.vertex_count * sizeof(OUTPUT_MESH_TEXTURE));
    m_Priv.shade_vbo_data =
        Memory_Alloc(m_Priv.vertex_count * sizeof(M_MESH_SHADE));

    glGenVertexArrays(1, &m_Priv.vao);
    glGenBuffers(1, &m_Priv.geom_vbo);
    glGenBuffers(1, &m_Priv.tex_vbo);
    glGenBuffers(1, &m_Priv.shade_vbo);

    glBindVertexArray(m_Priv.vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_Priv.geom_vbo);
    // attribute 0: position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE, sizeof(M_MESH_VERTEX),
        (void *)(intptr_t)offsetof(M_MESH_VERTEX, pos));

    // attribute 1: normal (ignore)

    // attribute 5: flags
    glEnableVertexAttribArray(5);
    glVertexAttribIPointer(
        5, 1, OUTPUT_USHORT_GL, sizeof(M_MESH_VERTEX),
        (void *)(intptr_t)offsetof(M_MESH_VERTEX, flags));

    // attribute 6: mesh color (ignore)

    glBindBuffer(GL_ARRAY_BUFFER, m_Priv.tex_vbo);
    // attribute 2: uvw
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(
        2, 3, GL_FLOAT, GL_FALSE, sizeof(OUTPUT_MESH_TEXTURE),
        (void *)(intptr_t)offsetof(OUTPUT_MESH_TEXTURE, uvw));

    // attribute 3: texture size
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(
        3, 4, GL_FLOAT, GL_FALSE, sizeof(OUTPUT_MESH_TEXTURE),
        (void *)(intptr_t)offsetof(OUTPUT_MESH_TEXTURE, texture_size));

    // attribute 4: trapezoid ratios
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(
        4, 2, GL_FLOAT, GL_FALSE, sizeof(OUTPUT_MESH_TEXTURE),
        (void *)(intptr_t)offsetof(OUTPUT_MESH_TEXTURE, trapezoid_ratio));

    glBindBuffer(GL_ARRAY_BUFFER, m_Priv.shade_vbo);

    // attribute 7 (shade)
    glEnableVertexAttribArray(7);
    glVertexAttribPointer(
        7, 1, OUTPUT_USHORT_GL, GL_FALSE, sizeof(M_MESH_SHADE), 0);

    M_UpdateVertices();
}

static void M_FreeBuffers(void)
{
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    if (m_Priv.vao != 0) {
        glDeleteVertexArrays(1, &m_Priv.vao);
        m_Priv.vao = 0;
    }
    if (m_Priv.geom_vbo != 0) {
        glDeleteBuffers(1, &m_Priv.geom_vbo);
        m_Priv.geom_vbo = 0;
    }
    if (m_Priv.tex_vbo != 0) {
        glDeleteBuffers(1, &m_Priv.tex_vbo);
        m_Priv.tex_vbo = 0;
    }
    if (m_Priv.shade_vbo != 0) {
        glDeleteBuffers(1, &m_Priv.shade_vbo);
        m_Priv.shade_vbo = 0;
    }
    Memory_FreePointer(&m_Priv.geom_vbo_data);
    Memory_FreePointer(&m_Priv.tex_vbo_data);
    Memory_FreePointer(&m_Priv.shade_vbo_data);
    for (int32_t i = 0; i < (int32_t)m_Priv.batch_count; i++) {
        Vector_Free(m_Priv.batches[i].animated_vertices);
        Memory_FreePointer(&m_Priv.batches[i].caustics);
    }
    Memory_FreePointer(&m_Priv.batches);
}

void Output_Meshes_InitRooms(void)
{
    m_Shader = Output_Meshes_GetShader();
    for (int32_t i = 0; i < WIBBLE_SIZE; i++) {
        const int16_t angle = (i * DEG_360) / WIBBLE_SIZE;
        m_ShadeTable[i] = Math_Sin(angle) * SHADE_CAUSTICS >> W2V_SHIFT;
        m_CausticsTable[i] = (Random_GetDraw() >> 5) - 0x01FF;
    }
}

void Output_Meshes_ShutdownRooms(void)
{
    M_FreeBuffers();
}

void Output_Meshes_ObserveLevelLoadRooms(void)
{
    M_FreeBuffers();
    M_PrepareBuffers();
}

static void M_FillAnimatedTextures(const ROOM *const room)
{
    const M_BATCH *const batch = M_GetBatch(room);
    OUTPUT_MESH_TEXTURE *out_texture =
        &m_Priv.tex_vbo_data[batch->vertex_start];
    for (int32_t j = 0; j < room->mesh.num_face4s; j++) {
        const FACE4 *const face = &room->mesh.face4s[j];
        if (!Output_Textures_IsObjectTextureAnimated(face->texture_idx)) {
            out_texture += OUTPUT_QUAD_VERTICES;
            continue;
        }
        for (int32_t k = 0; k < OUTPUT_QUAD_VERTICES; k++) {
            const int32_t l = OUTPUT_QUAD_TO_FAN(k);
            const int32_t uvw_idx = face->texture_idx * 4 + l;
            M_FillTexture(
                out_texture, uvw_idx, face->texture_zw[l].z,
                face->texture_zw[l].w);
            out_texture++;
        }
    }
    for (int32_t j = 0; j < room->mesh.num_face3s; j++) {
        const FACE3 *const face = &room->mesh.face3s[j];
        if (!Output_Textures_IsObjectTextureAnimated(face->texture_idx)) {
            out_texture += OUTPUT_TRI_VERTICES;
            continue;
        }
        for (int32_t k = 0; k < OUTPUT_TRI_VERTICES; k++) {
            const int32_t l = OUTPUT_TRI_TO_FAN(k);
            const int32_t uvw_idx = face->texture_idx * 4 + l;
            M_FillTexture(out_texture, uvw_idx, 1.0f, 1.0f);
            out_texture++;
        }
    }
}

static void M_UpdateAnimatedTextures(const ROOM *const room)
{
    const M_BATCH *const batch = M_GetBatch(room);
    glBindBuffer(GL_ARRAY_BUFFER, m_Priv.tex_vbo);
    for (int32_t i = 0; i < batch->animated_vertices->count; i++) {
        const OUTPUT_VERTEX_RANGE *const range =
            Vector_Get(batch->animated_vertices, i);
        GFX_TRACK_DATA(
            glBufferSubData, GL_ARRAY_BUFFER,
            range->vertex_start * sizeof(OUTPUT_MESH_TEXTURE),
            range->vertex_count * sizeof(OUTPUT_MESH_TEXTURE),
            &m_Priv.tex_vbo_data[range->vertex_start]);
    }
}

void Output_Meshes_ObserveTextureAnimationRooms(void)
{
    for (int32_t i = 0; i < Room_GetCount(); i++) {
        const ROOM *const room = Room_Get(i);
        M_FillAnimatedTextures(room);
        M_UpdateAnimatedTextures(room);
    }
}

void Output_Meshes_ObserveRoomFlip(const ROOM *const room)
{
    M_FillAnimatedTextures(room);
    M_UpdateAnimatedTextures(room);
}

void Output_Meshes_RenderRoomMesh(
    const MATRIX *const matrix, const RGB_F tint, const ROOM *const room)
{
    const M_BATCH *const batch = M_GetBatch(room);

    M_UpdateRoomShades(room);

    Output_Shader_UploadMatrix(m_Shader, matrix);
    Output_Shader_UploadTint(m_Shader, tint);
    GFX_GL_CheckError();

    glBindBuffer(GL_ARRAY_BUFFER, m_Priv.shade_vbo);
    GFX_TRACK_SUBDATA(
        glBufferSubData, GL_ARRAY_BUFFER,
        batch->vertex_start * sizeof(M_MESH_SHADE),
        batch->vertex_count * sizeof(M_MESH_SHADE),
        &m_Priv.shade_vbo_data[batch->vertex_start]);
    GFX_GL_CheckError();

    glEnable(GL_CULL_FACE);
    glBindVertexArray(m_Priv.vao);
    GFX_GL_CheckError();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, Output_Textures_GetAtlasTexture());
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, Output_Textures_GetEnvMapTexture());
    GFX_GL_CheckError();

    if (Output_GetWibbleEffect()) {
        Output_Shader_UploadWibbleEffect(m_Shader, false);
        glDepthMask(GL_FALSE);
        glDrawArrays(GL_TRIANGLES, batch->vertex_start, batch->vertex_count);
        glDepthMask(GL_TRUE);
        Output_Shader_UploadWibbleEffect(m_Shader, true);
        glDrawArrays(GL_TRIANGLES, batch->vertex_start, batch->vertex_count);
    } else {
        Output_Shader_UploadWibbleEffect(m_Shader, false);
        glDrawArrays(GL_TRIANGLES, batch->vertex_start, batch->vertex_count);
    }

    glDisable(GL_CULL_FACE);
    GFX_GL_CheckError();
}
