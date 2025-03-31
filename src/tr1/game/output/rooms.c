#include "game/output/rooms.h"

#include "game/output.h"
#include "game/output/meshes.h"
#include "game/output/textures.h"
#include "game/output/utils.h"
#include "game/room.h"

#include <libtrx/gfx/gl/utils.h>
#include <libtrx/memory.h>

#pragma pack(push, 1)
typedef struct {
    // attribute 0
    XYZ_F pos;

    // attribute 1
    int32_t uvw_idx;

    // attribute 2
    float trapezoid_ratio[2];

    // attribute 3
    uint16_t flags;
} M_MESH_VERTEX;

typedef uint16_t M_MESH_SHADE;
#pragma pack(pop)

typedef struct {
    int32_t vertex_start;
    int32_t vertex_count;
} M_ROOM_BATCH;

static struct {
    GLuint vao;
    GLuint geom_vbo;
    size_t vertex_count;
    GLuint shade_vbo;
    M_MESH_VERTEX *geom_vbo_data;
    M_MESH_SHADE *shade_vbo_data;
    size_t batch_count;
    M_ROOM_BATCH *batches;
    size_t last_vertex;
} m_Priv;

static OUTPUT_SHADER *m_Shader = nullptr;

static M_ROOM_BATCH *M_GetBatch(const ROOM *const room)
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
    out_vertex->flags = flags;
}

static void M_AppendRoomFace4(const ROOM *const room, const FACE4 *const face)
{
    for (int32_t i = 0; i < OUTPUT_QUAD_VERTICES; i++) {
        const int32_t j = OUTPUT_QUAD_TO_FAN(i);
        const int32_t uvw_idx = face->texture_idx * 4 + j;
        const ROOM_VERTEX *const room_vertex =
            &room->mesh.vertices[face->vertices[j]];
        M_MESH_VERTEX *const out_vertex =
            &m_Priv.geom_vbo_data[m_Priv.last_vertex];
        M_FillVertex(out_vertex, uvw_idx, room_vertex->pos, room_vertex->flags);
        out_vertex->trapezoid_ratio[0] = face->texture_zw[j].z;
        out_vertex->trapezoid_ratio[1] = face->texture_zw[j].w;
        m_Priv.last_vertex++;
    }
}

static void M_AppendRoomFace3(const ROOM *const room, const FACE3 *const face)
{
    for (int32_t i = 0; i < OUTPUT_TRI_VERTICES; i++) {
        const int32_t j = OUTPUT_TRI_TO_FAN(i);
        const int32_t uvw_idx = face->texture_idx * 4 + j;
        const ROOM_VERTEX *const room_vertex =
            &room->mesh.vertices[face->vertices[j]];
        M_MESH_VERTEX *const out_vertex =
            &m_Priv.geom_vbo_data[m_Priv.last_vertex];
        M_FillVertex(out_vertex, uvw_idx, room_vertex->pos, room_vertex->flags);
        out_vertex->trapezoid_ratio[0] = 1.0f;
        out_vertex->trapezoid_ratio[1] = 1.0f;
        m_Priv.last_vertex++;
    }
}

static void M_UpdateRoomGeometry(const ROOM *const room)
{
    M_ROOM_BATCH *const batch = &m_Priv.batches[Room_GetNumber(room)];
    batch->vertex_start = m_Priv.last_vertex;
    for (int32_t i = 0; i < room->mesh.num_face4s; i++) {
        M_AppendRoomFace4(room, &room->mesh.face4s[i]);
    }
    for (int32_t i = 0; i < room->mesh.num_face3s; i++) {
        M_AppendRoomFace3(room, &room->mesh.face3s[i]);
    }
    batch->vertex_count = m_Priv.last_vertex - batch->vertex_start;
}

static void M_UpdateRoomShades(const ROOM *const room)
{
    M_ROOM_BATCH *const batch = M_GetBatch(room);
    int32_t v = batch->vertex_start;
    for (int32_t i = 0; i < room->mesh.num_face4s; i++) {
        const FACE4 *const face = &room->mesh.face4s[i];
        for (int32_t j = 0; j < OUTPUT_QUAD_VERTICES; j++) {
            const int32_t k = OUTPUT_QUAD_TO_FAN(j);
            m_Priv.shade_vbo_data[v] =
                room->mesh.vertices[face->vertices[k]].light_adder;
            v++;
        }
    }
    for (int32_t i = 0; i < room->mesh.num_face3s; i++) {
        const FACE3 *const face = &room->mesh.face3s[i];
        for (int32_t j = 0; j < OUTPUT_TRI_VERTICES; j++) {
            const int32_t k = OUTPUT_TRI_TO_FAN(j);
            m_Priv.shade_vbo_data[v] =
                room->mesh.vertices[face->vertices[k]].light_adder;
            v++;
        }
    }
}

static void M_UpdateRoomVertices(void)
{
    m_Priv.last_vertex = 0;

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
    m_Priv.batches = Memory_Realloc(
        m_Priv.batches, sizeof(M_ROOM_BATCH) * m_Priv.batch_count);

    int32_t num_vertices = 0;
    for (int32_t i = 0; i < Room_GetCount(); i++) {
        num_vertices += Room_Get(i)->mesh.num_face4s * 6;
        num_vertices += Room_Get(i)->mesh.num_face3s * 3;
    }

    m_Priv.vertex_count = num_vertices;

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

    glEnableVertexAttribArray(1);
    glVertexAttribIPointer(
        1, 1, GL_UNSIGNED_INT, sizeof(M_MESH_VERTEX),
        (void *)(intptr_t)offsetof(M_MESH_VERTEX, uvw_idx));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(
        2, 2, GL_FLOAT, GL_FALSE, sizeof(M_MESH_VERTEX),
        (void *)(intptr_t)offsetof(M_MESH_VERTEX, trapezoid_ratio));

    glEnableVertexAttribArray(3);
    glVertexAttribIPointer(
        3, 1, GL_UNSIGNED_SHORT, sizeof(M_MESH_VERTEX),
        (void *)(intptr_t)offsetof(M_MESH_VERTEX, flags));

    glBindBuffer(GL_ARRAY_BUFFER, m_Priv.shade_vbo);
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(
        4, 1, GL_UNSIGNED_SHORT, GL_FALSE, sizeof(M_MESH_SHADE), 0);

    M_UpdateRoomVertices();
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
    Memory_FreePointer(&m_Priv.batches);
}

void Output_Rooms_Init(void)
{
    m_Shader = Output_Meshes_GetShader();
}

void Output_Rooms_Shutdown(void)
{
    M_FreeBuffers();
}

void Output_Rooms_ObserveLevelLoad(void)
{
    M_FreeBuffers();
    M_PrepareBuffers();
}

void Output_Rooms_RenderRoom(
    const MATRIX *const matrix, const RGB_F tint, const ROOM *const room)
{
    const M_ROOM_BATCH *const batch = M_GetBatch(room);

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
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_BUFFER, Output_Textures_GetObjectUVWsTexture());
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, Output_Textures_GetAtlasTexture());

    if (Output_GetWibbleOffset() >= 0) {
        Output_Shader_UploadWibble(m_Shader, -1);
        glDepthMask(GL_FALSE);
        glDrawArrays(GL_TRIANGLES, batch->vertex_start, batch->vertex_count);
        glDepthMask(GL_TRUE);
    }
    Output_Shader_UploadWibble(m_Shader, Output_GetWibbleOffset());
    glDrawArrays(GL_TRIANGLES, batch->vertex_start, batch->vertex_count);

    glDisable(GL_CULL_FACE);
    GFX_GL_CheckError();
}
