#include "game/output/meshes/dynamic.h"

#include "game/output/meshes/common.h"

#include <libtrx/gfx/gl/utils.h>
#include <libtrx/memory.h>

#include <string.h>

static struct {
    GLuint vao;
    GLuint vbo;
} m_Priv;

static OUTPUT_SHADER *m_Shader = nullptr;

void Output_Meshes_InitDynamic(void)
{
    m_Shader = Output_Meshes_GetShader();

    glGenVertexArrays(1, &m_Priv.vao);
    glBindVertexArray(m_Priv.vao);

    glGenBuffers(1, &m_Priv.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_Priv.vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE, sizeof(OUTPUT_MESH_VERTEX),
        (void *)(intptr_t)offsetof(OUTPUT_MESH_VERTEX, pos));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1, 3, GL_FLOAT, GL_FALSE, sizeof(OUTPUT_MESH_VERTEX),
        (void *)(intptr_t)offsetof(OUTPUT_MESH_VERTEX, normal));

    glEnableVertexAttribArray(2);
    glVertexAttribIPointer(
        2, 1, GL_UNSIGNED_INT, sizeof(OUTPUT_MESH_VERTEX),
        (void *)(intptr_t)offsetof(OUTPUT_MESH_VERTEX, uvw_idx));

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(
        3, 2, GL_FLOAT, GL_FALSE, sizeof(OUTPUT_MESH_VERTEX),
        (void *)(intptr_t)offsetof(OUTPUT_MESH_VERTEX, trapezoid_ratio));

    glEnableVertexAttribArray(4);
    glVertexAttribIPointer(
        4, 1, GL_UNSIGNED_SHORT, sizeof(OUTPUT_MESH_VERTEX),
        (void *)(intptr_t)offsetof(OUTPUT_MESH_VERTEX, flags));

    glEnableVertexAttribArray(5);
    glVertexAttribPointer(
        5, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(OUTPUT_MESH_VERTEX),
        (void *)(intptr_t)offsetof(OUTPUT_MESH_VERTEX, color));

    glEnableVertexAttribArray(6);
    glVertexAttribPointer(
        6, 1, GL_UNSIGNED_SHORT, GL_FALSE, sizeof(OUTPUT_MESH_VERTEX),
        (void *)(intptr_t)offsetof(OUTPUT_MESH_VERTEX, shade));
}

void Output_Meshes_ShutdownDynamic(void)
{
    if (m_Priv.vao != 0) {
        glDeleteVertexArrays(1, &m_Priv.vao);
        m_Priv.vao = 0;
    }
    if (m_Priv.vbo != 0) {
        glDeleteBuffers(1, &m_Priv.vbo);
        m_Priv.vbo = 0;
    }
}

void Output_Meshes_DrawPrimitives(
    const GLuint prim_type, const int32_t count,
    const OUTPUT_MESH_VERTEX *const vertices)
{
    Output_Shader_Bind(m_Shader);
    glBindBuffer(GL_ARRAY_BUFFER, m_Priv.vbo);
    GFX_TRACK_DATA(
        glBufferData, GL_ARRAY_BUFFER, count * sizeof(OUTPUT_MESH_VERTEX),
        vertices, GL_STATIC_DRAW);
    glBindVertexArray(m_Priv.vao);
    glDrawArrays(prim_type, 0, count);
}

void Output_Meshes_DrawTriangles(
    const int32_t count, const OUTPUT_MESH_VERTEX *const vertices)
{
    Output_Meshes_DrawPrimitives(GL_TRIANGLES, count, vertices);
}
