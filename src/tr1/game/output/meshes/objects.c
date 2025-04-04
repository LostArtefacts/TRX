#include "game/output/meshes/objects.h"

#include "game/output.h"
#include "game/output/meshes/common.h"
#include "game/output/textures.h"
#include "game/output/utils.h"

#include <libtrx/debug.h>
#include <libtrx/game/output/objects.h>
#include <libtrx/gfx/gl/utils.h>
#include <libtrx/memory.h>

#pragma pack(push, 1)
typedef struct {
    // clang-format off
    XYZ_F pos;                // attribute 0
    XYZ_F normal;             // attribute 1
    int32_t uvw_idx;          // attribute 2
    float trapezoid_ratio[2]; // attribute 3
    uint16_t flags;           // attribute 4
    RGBA_8888 color;          // attribute 5
    // clang-format on
} M_MESH_VERTEX;

typedef int16_t M_MESH_SHADE;
#pragma pack(pop)

typedef struct {
    int32_t vertex_start;
    int32_t vertex_count;
} M_BATCH;

static struct {
    GLuint vao;
    GLuint geom_vbo;
    size_t vertex_count;
    GLuint shade_vbo;
    M_MESH_VERTEX *geom_vbo_data;
    M_MESH_SHADE *shade_vbo_data;
    int32_t *light_idx_map;
    size_t batch_count;
    M_BATCH *batches;
} m_Priv;

static OUTPUT_SHADER *m_Shader = nullptr;

static M_BATCH *M_GetBatch(const OBJECT_MESH *const mesh)
{
    const int16_t mesh_num = Object_GetMeshIndex(mesh);
    return &m_Priv.batches[mesh_num];
}

static uint16_t M_GetFlags(const OBJECT_MESH *const mesh, const bool flat)
{
    uint16_t flags = 0;
    if (flat) {
        flags |= VERT_FLAT_SHADED;
    }
    if (mesh->enable_reflections) {
        flags |= VERT_REFLECTIVE;
    }
    if (mesh->disable_lighting) {
        flags |= VERT_NO_LIGHTING;
    }
    return flags;
}

static void M_FillVertex(
    M_MESH_VERTEX *const out_vertex, const int32_t uvw_idx,
    const OBJECT_MESH *const mesh, const int32_t vertex_idx,
    const int32_t palette_idx)
{
    const XYZ_16 pos = mesh->vertices[vertex_idx];
    const XYZ_16 normal = mesh->lighting.normals[vertex_idx];
    out_vertex->pos = (XYZ_F) { .x = pos.x, .y = pos.y, .z = pos.z };
    out_vertex->normal =
        (XYZ_F) { .x = normal.x, .y = -normal.y, .z = normal.z };
    out_vertex->uvw_idx = uvw_idx;
    out_vertex->flags = M_GetFlags(mesh, palette_idx >= 0);
    if (palette_idx >= 0) {
        out_vertex->color =
            Output_RGB2RGBA(Output_GetPaletteColor8(palette_idx));
    } else {
        out_vertex->color = (RGBA_8888) { 255, 255, 255, 255 };
    }
}

static M_MESH_VERTEX *M_FillFace4(
    const OBJECT_MESH *const mesh, M_MESH_VERTEX *out_vertex,
    const FACE4 *const face, const bool flat)
{
    for (int32_t i = 0; i < OUTPUT_QUAD_VERTICES; i++) {
        const int32_t j = OUTPUT_QUAD_TO_FAN(i);
        const int32_t uvw_idx = face->texture_idx * 4 + j;
        M_FillVertex(
            out_vertex, uvw_idx, mesh, face->vertices[j],
            flat ? face->palette_idx : -1);
        out_vertex->trapezoid_ratio[0] = face->texture_zw[j].z;
        out_vertex->trapezoid_ratio[1] = face->texture_zw[j].w;
        out_vertex++;
    }
    return out_vertex;
}

static M_MESH_VERTEX *M_FillFace3(
    const OBJECT_MESH *const mesh, M_MESH_VERTEX *out_vertex,
    const FACE3 *const face, const bool flat)
{
    const M_BATCH *const batch = M_GetBatch(mesh);
    for (int32_t i = 0; i < OUTPUT_TRI_VERTICES; i++) {
        const int32_t j = OUTPUT_TRI_TO_FAN(i);
        const int32_t uvw_idx = face->texture_idx * 4 + j;
        M_FillVertex(
            out_vertex, uvw_idx, mesh, face->vertices[j],
            flat ? face->palette_idx : -1);
        out_vertex->trapezoid_ratio[0] = 1.0f;
        out_vertex->trapezoid_ratio[1] = 1.0f;
        out_vertex++;
    }
    return out_vertex;
}

static void M_UpdateGeometry(const OBJECT_MESH *const mesh)
{
    const M_BATCH *const batch = M_GetBatch(mesh);
    M_MESH_VERTEX *out_vertex = &m_Priv.geom_vbo_data[batch->vertex_start];

    for (int32_t i = 0; i < mesh->num_tex_face4s; i++) {
        out_vertex = M_FillFace4(mesh, out_vertex, &mesh->tex_face4s[i], false);
    }
    for (int32_t i = 0; i < mesh->num_tex_face3s; i++) {
        out_vertex = M_FillFace3(mesh, out_vertex, &mesh->tex_face3s[i], false);
    }
    for (int32_t i = 0; i < mesh->num_flat_face4s; i++) {
        out_vertex = M_FillFace4(mesh, out_vertex, &mesh->flat_face4s[i], true);
    }
    for (int32_t i = 0; i < mesh->num_flat_face3s; i++) {
        out_vertex = M_FillFace3(mesh, out_vertex, &mesh->flat_face3s[i], true);
    }

#define SET(v, j) light_idx_map[v] = face->vertices[j];
    int32_t *const light_idx_map = &m_Priv.light_idx_map[batch->vertex_start];
    int32_t v = 0;
    for (int32_t i = 0; i < mesh->num_tex_face4s; i++) {
        const FACE4 *const face = &mesh->tex_face4s[i];
        for (int32_t j = 0; j < OUTPUT_QUAD_VERTICES; j++) {
            SET(v, OUTPUT_QUAD_TO_FAN(j));
            v++;
        }
    }
    for (int32_t i = 0; i < mesh->num_tex_face3s; i++) {
        const FACE3 *const face = &mesh->tex_face3s[i];
        for (int32_t j = 0; j < OUTPUT_TRI_VERTICES; j++) {
            SET(v, OUTPUT_TRI_TO_FAN(j));
            v++;
        }
    }
    for (int32_t i = 0; i < mesh->num_flat_face4s; i++) {
        const FACE4 *const face = &mesh->flat_face4s[i];
        for (int32_t j = 0; j < OUTPUT_QUAD_VERTICES; j++) {
            SET(v, OUTPUT_QUAD_TO_FAN(j));
            v++;
        }
    }
    for (int32_t i = 0; i < mesh->num_flat_face3s; i++) {
        const FACE3 *const face = &mesh->flat_face3s[i];
        for (int32_t j = 0; j < OUTPUT_TRI_VERTICES; j++) {
            SET(v, OUTPUT_TRI_TO_FAN(j));
            v++;
        }
    }
#undef SET
}

static void M_UpdateShades(
    const MATRIX *const matrix, const OBJECT_MESH *const mesh)
{
    M_BATCH *const batch = M_GetBatch(mesh);
    M_MESH_SHADE *const out_shades =
        &m_Priv.shade_vbo_data[batch->vertex_start];
    int32_t *const light_idx_map = &m_Priv.light_idx_map[batch->vertex_start];

    const int32_t ls_adder = Output_GetLightAdder();
    const int32_t ls_divider = Output_GetLightDivider();
    const XYZ_32 ls_vector_view = Output_GetLightVectorView();

    if (mesh->num_lights <= 0) {
        for (int32_t i = 0; i < batch->vertex_count; i++) {
            const int32_t j = light_idx_map[i];
            int16_t shade = ls_adder + mesh->lighting.lights[j];
            CLAMP(shade, 0, 0x1FFF);
            out_shades[i] = shade;
        }
    } else if (ls_divider == 0) {
        int16_t shade = ls_adder;
        CLAMP(shade, 0, 0x1FFF);
        for (int32_t i = 0; i < batch->vertex_count; i++) {
            out_shades[i] = shade;
        }
    } else {
        // clang-format off
        const int32_t xv = (
            matrix->_00 * ls_vector_view.x +
            matrix->_10 * ls_vector_view.y +
            matrix->_20 * ls_vector_view.z
        ) / ls_divider;

        const int32_t yv = (
            matrix->_01 * ls_vector_view.x +
            matrix->_11 * ls_vector_view.y +
            matrix->_21 * ls_vector_view.z
        ) / ls_divider;

        const int32_t zv = (
            matrix->_02 * ls_vector_view.x +
            matrix->_12 * ls_vector_view.y +
            matrix->_22 * ls_vector_view.z
        ) / ls_divider;
        // clang-format on

        for (int32_t i = 0; i < batch->vertex_count; i++) {
            const int32_t j = light_idx_map[i];
            const XYZ_16 *const normal = &mesh->lighting.normals[j];
            int16_t shade = ls_adder
                + ((normal->x * xv + normal->y * yv + normal->z * zv) >> 16);
            CLAMP(shade, 0, 0x1FFF);
            out_shades[i] = shade;
        }
    }
}

static void M_UpdateVertices(void)
{
    for (int32_t i = 0; i < Object_GetMeshCount(); i++) {
        M_UpdateGeometry(Object_GetMesh(i));
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
    m_Priv.batch_count = Object_GetMeshCount();
    m_Priv.batches =
        Memory_Realloc(m_Priv.batches, sizeof(M_BATCH) * m_Priv.batch_count);

    int32_t last_vertex = 0;
    for (int32_t i = 0; i < Object_GetMeshCount(); i++) {
        const OBJECT_MESH *const obj_mesh = Object_GetMesh(i);
        M_BATCH *const batch = &m_Priv.batches[i];
        batch->vertex_start = last_vertex;
        batch->vertex_count = 0;
        batch->vertex_count += obj_mesh->num_tex_face4s * OUTPUT_QUAD_VERTICES;
        batch->vertex_count += obj_mesh->num_tex_face3s * OUTPUT_TRI_VERTICES;
        batch->vertex_count += obj_mesh->num_flat_face4s * OUTPUT_QUAD_VERTICES;
        batch->vertex_count += obj_mesh->num_flat_face3s * OUTPUT_TRI_VERTICES;
        last_vertex += batch->vertex_count;
    }
    m_Priv.vertex_count = last_vertex;

    m_Priv.geom_vbo_data =
        Memory_Alloc(m_Priv.vertex_count * sizeof(M_MESH_VERTEX));
    m_Priv.shade_vbo_data =
        Memory_Alloc(m_Priv.vertex_count * sizeof(M_MESH_SHADE));
    m_Priv.light_idx_map = Memory_Alloc(m_Priv.vertex_count * sizeof(int32_t));

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
    glVertexAttribPointer(
        1, 3, GL_FLOAT, GL_FALSE, sizeof(M_MESH_VERTEX),
        (void *)(intptr_t)offsetof(M_MESH_VERTEX, normal));

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

    glEnableVertexAttribArray(5);
    glVertexAttribPointer(
        5, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(M_MESH_VERTEX),
        (void *)(intptr_t)offsetof(M_MESH_VERTEX, color));

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
    Memory_FreePointer(&m_Priv.light_idx_map);
    Memory_FreePointer(&m_Priv.batches);
}

void Output_Meshes_InitObjects(void)
{
    m_Shader = Output_Meshes_GetShader();
}

void Output_Meshes_ShutdownObjects(void)
{
    M_FreeBuffers();
}

void Output_Meshes_ObserveLevelLoadObjects(void)
{
    M_PrepareBuffers();
}

void Output_Meshes_ObserveLevelUnloadObjects(void)
{
    M_FreeBuffers();
}

void Output_Meshes_ObserveObjectMeshSwap(
    const OBJECT_MESH *const mesh_1, const OBJECT_MESH *const mesh_2)
{
    if (m_Priv.geom_vbo_data == nullptr) {
        return;
    }

    M_BATCH *const batch_1 = M_GetBatch(mesh_1);
    M_BATCH *const batch_2 = M_GetBatch(mesh_2);
    const M_BATCH batch_tmp = *batch_1;
    *batch_1 = *batch_2;
    *batch_2 = batch_tmp;
    Output_Meshes_ObserveObjectMeshUpdate(mesh_1);
    Output_Meshes_ObserveObjectMeshUpdate(mesh_2);
}

void Output_Meshes_ObserveObjectMeshUpdate(const OBJECT_MESH *const mesh)
{
    if (m_Priv.geom_vbo_data == nullptr) {
        return;
    }

    const M_BATCH *const batch = M_GetBatch(mesh);
    M_UpdateGeometry(mesh);
    glBindBuffer(GL_ARRAY_BUFFER, m_Priv.geom_vbo);
    GFX_TRACK_SUBDATA(
        glBufferSubData, GL_ARRAY_BUFFER,
        batch->vertex_start * sizeof(M_MESH_VERTEX),
        batch->vertex_count * sizeof(M_MESH_VERTEX),
        &m_Priv.geom_vbo_data[batch->vertex_start]);
    GFX_GL_CheckError();
}

void Output_Meshes_RenderObjectMesh(
    const MATRIX *const matrix, const RGB_F tint, const OBJECT_MESH *const mesh)
{
    const M_BATCH *const batch = M_GetBatch(mesh);

    M_UpdateShades(matrix, mesh);

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
    GFX_GL_CheckError();

    Output_Shader_UploadWibbleEffect(m_Shader, false);
    glDrawArrays(GL_TRIANGLES, batch->vertex_start, batch->vertex_count);

    glDisable(GL_CULL_FACE);
    GFX_GL_CheckError();
}
