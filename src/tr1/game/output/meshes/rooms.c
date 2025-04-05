#include "game/output/meshes/rooms.h"

#include "game/output.h"
#include "game/output/meshes/common.h"
#include "game/output/textures.h"
#include "game/output/utils.h"
#include "game/random.h"
#include "game/room.h"

#include <libtrx/gfx/gl/utils.h>
#include <libtrx/memory.h>

#pragma pack(push, 1)
typedef struct {
    // clang-format off
    XYZ_F pos;                // attribute 0
    int32_t uvw_idx;          // attribute 2
    float trapezoid_ratio[2]; // attribute 3
    uint16_t flags;           // attribute 4
    // clang-format on
} M_MESH_VERTEX;

typedef int16_t M_MESH_SHADE;
#pragma pack(pop)

typedef struct {
    int32_t vertex_start;
    int32_t vertex_count;
    int16_t *caustics;
} M_BATCH;

static struct {
    GLuint vao;
    GLuint geom_vbo;
    size_t vertex_count;
    GLuint shade_vbo;
    M_MESH_VERTEX *geom_vbo_data;
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
        CLAMP(source, 0, MAX_LIGHTING);
    } else {
        CLAMPG(source, MAX_LIGHTING);
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
    M_MESH_VERTEX *const out_vertex, const int32_t uvw_idx, const XYZ_16 pos,
    const uint16_t flags)
{
    out_vertex->pos = (XYZ_F) { .x = pos.x, .y = pos.y, .z = pos.z };
    out_vertex->uvw_idx = uvw_idx;
    out_vertex->flags = flags & NO_VERT_MOVE ? VERT_NO_CAUSTICS : 0;
}

static M_MESH_VERTEX *M_FillRoomFace4(
    const ROOM *const room, M_MESH_VERTEX *out_vertex, const FACE4 *const face)
{
    for (int32_t i = 0; i < OUTPUT_QUAD_VERTICES; i++) {
        const int32_t j = OUTPUT_QUAD_TO_FAN(i);
        const int32_t uvw_idx = face->texture_idx * 4 + j;
        const ROOM_VERTEX *const room_vertex =
            &room->mesh.vertices[face->vertices[j]];
        M_FillVertex(out_vertex, uvw_idx, room_vertex->pos, room_vertex->flags);
        out_vertex->trapezoid_ratio[0] = face->texture_zw[j].z;
        out_vertex->trapezoid_ratio[1] = face->texture_zw[j].w;
        out_vertex++;
    }
    return out_vertex;
}

static M_MESH_VERTEX *M_FillRoomFace3(
    const ROOM *const room, M_MESH_VERTEX *out_vertex, const FACE3 *const face)
{
    for (int32_t i = 0; i < OUTPUT_TRI_VERTICES; i++) {
        const int32_t j = OUTPUT_TRI_TO_FAN(i);
        const int32_t uvw_idx = face->texture_idx * 4 + j;
        const ROOM_VERTEX *const room_vertex =
            &room->mesh.vertices[face->vertices[j]];
        M_FillVertex(out_vertex, uvw_idx, room_vertex->pos, room_vertex->flags);
        out_vertex->trapezoid_ratio[0] = 1.0f;
        out_vertex->trapezoid_ratio[1] = 1.0f;
        out_vertex++;
    }
    return out_vertex;
}

static void M_UpdateRoomGeometry(const ROOM *const room)
{
    M_BATCH *const batch = &m_Priv.batches[Room_GetNumber(room)];
    M_MESH_VERTEX *out_vertex = &m_Priv.geom_vbo_data[batch->vertex_start];
    for (int32_t i = 0; i < room->mesh.num_face4s; i++) {
        out_vertex = M_FillRoomFace4(room, out_vertex, &room->mesh.face4s[i]);
    }
    for (int32_t i = 0; i < room->mesh.num_face3s; i++) {
        out_vertex = M_FillRoomFace3(room, out_vertex, &room->mesh.face3s[i]);
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
        batch->vertex_count +=
            Room_Get(i)->mesh.num_face4s * OUTPUT_QUAD_VERTICES;
        batch->vertex_count +=
            Room_Get(i)->mesh.num_face3s * OUTPUT_TRI_VERTICES;
        last_vertex += batch->vertex_count;

        for (int32_t j = 0; j < room->mesh.num_vertices; j++) {
            batch->caustics[j] =
                m_CausticsTable[(room->mesh.num_vertices - j) % WIBBLE_SIZE];
        }
    }

    m_Priv.vertex_count = last_vertex;

    m_Priv.geom_vbo_data =
        Memory_Alloc(m_Priv.vertex_count * sizeof(M_MESH_VERTEX));
    m_Priv.shade_vbo_data =
        Memory_Alloc(m_Priv.vertex_count * sizeof(M_MESH_SHADE));

    glGenVertexArrays(1, &m_Priv.vao);
    glGenBuffers(1, &m_Priv.geom_vbo);
    glGenBuffers(1, &m_Priv.shade_vbo);

    glBindBuffer(GL_ARRAY_BUFFER, m_Priv.geom_vbo);
    glBindVertexArray(m_Priv.vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_Priv.geom_vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE, sizeof(M_MESH_VERTEX),
        (void *)(intptr_t)offsetof(M_MESH_VERTEX, pos));

    // ignore attribute 1 (normals) - rooms never have reflective faces

    glEnableVertexAttribArray(2);
    glVertexAttribIPointer(
        2, 1, GL_UNSIGNED_INT, sizeof(M_MESH_VERTEX),
        (void *)(intptr_t)offsetof(M_MESH_VERTEX, uvw_idx));

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(
        3, 2, GL_FLOAT, GL_FALSE, sizeof(M_MESH_VERTEX),
        (void *)(intptr_t)offsetof(M_MESH_VERTEX, trapezoid_ratio));

    glEnableVertexAttribArray(4);
    glVertexAttribIPointer(
        4, 1, GL_UNSIGNED_SHORT, sizeof(M_MESH_VERTEX),
        (void *)(intptr_t)offsetof(M_MESH_VERTEX, flags));

    // ignore attribute 5 (mesh color) - rooms only ever use textured faces

    glBindBuffer(GL_ARRAY_BUFFER, m_Priv.shade_vbo);
    glEnableVertexAttribArray(6);
    glVertexAttribPointer(
        6, 1, GL_UNSIGNED_SHORT, GL_FALSE, sizeof(M_MESH_SHADE), 0);

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
    if (m_Priv.shade_vbo != 0) {
        glDeleteBuffers(1, &m_Priv.shade_vbo);
        m_Priv.shade_vbo = 0;
    }
    Memory_FreePointer(&m_Priv.geom_vbo_data);
    Memory_FreePointer(&m_Priv.shade_vbo_data);
    for (int32_t i = 0; i < (int32_t)m_Priv.batch_count; i++) {
        Memory_FreePointer(&m_Priv.batches[i].caustics);
    }
    Memory_FreePointer(&m_Priv.batches);
}

void Output_Meshes_InitRooms(void)
{
    m_Shader = Output_Meshes_GetShader();
    for (int32_t i = 0; i < WIBBLE_SIZE; i++) {
        const int16_t angle = (i * DEG_360) / WIBBLE_SIZE;
        m_ShadeTable[i] = Math_Sin(angle) * MAX_SHADE >> W2V_SHIFT;
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
    glBindTexture(GL_TEXTURE_BUFFER, Output_Textures_GetObjectUVWsTexture());
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, Output_Textures_GetEnvMapTexture());
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_BUFFER, Output_Textures_GetAtlasSizesTexture());
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
