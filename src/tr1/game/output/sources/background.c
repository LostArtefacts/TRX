#include "game/output/sources/background.h"

#include "game/output.h"
#include "game/output/scene_compositor.h"
#include "game/output/shader.h"
#include "game/output/textures.h"
#include "game/output/utils.h"
#include "game/viewport.h"

#include <libtrx/vector.h>

typedef struct {
    XYZ_F pos;
    OUTPUT_UVW uvw;
} M_VERTEX;

typedef struct {
    SCENE_SOURCE source;
    OUTPUT_SHADER *shader;
    VECTOR *vertex_indices;
    bool staged;
    GLuint vao;
    GLuint vbo;
} M_PRIV;

static M_PRIV m_Priv;

static void M_PrepareVertexGrid(
    const M_PRIV *p, size_t repeat_x, size_t repeat_y);

static void M_RenderBegin(const SCENE_SOURCE *source);
static void M_RenderPass(const SCENE_SOURCE *source, SCENE_PASS pass);
static bool M_IsDirty(const SCENE_SOURCE *source, SCENE_PASS pass);

static void M_PrepareVertexGrid(
    const M_PRIV *const p, const size_t repeat_x, const size_t repeat_y)
{
    const VIEWPORT_RECT vp = Viewport_GetRect(VIEWPORT_GAME);
    const float sx = vp.w / (float)repeat_x;
    const float sy = vp.h / (float)repeat_y;
    const size_t quad_offset[4][2] = { { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 } };

    Vector_Clear(p->vertex_indices);

    VECTOR *const vertices =
        Vector_CreateAtCapacity(sizeof(M_VERTEX), repeat_x * repeat_y * 4);
    for (size_t y = 0; y < repeat_y; y++) {
        for (size_t x = 0; x < repeat_x; x++) {
            const size_t start = vertices->count;
            for (size_t i = 0; i < 4; i++) {
                const size_t x2 = quad_offset[i][0];
                const size_t y2 = quad_offset[i][1];
                const M_VERTEX vertex = {
                    .pos = {
                        .x = (x + x2) * sx,
                        .y = (y + y2) * sy,
                        .z = Output_GetNearZ_UI(),
                    },
                    .uvw = { .u = x2, .v = y2 },
                };
                Vector_Add(vertices, &vertex);
            }
            for (size_t i = 0; i < OUTPUT_QUAD_VERTICES; i++) {
                Vector_Add(
                    p->vertex_indices,
                    &(int32_t) { start + OUTPUT_QUAD_TO_FAN(i) });
            }
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, p->vbo);
    GFX_TRACK_DATA(
        glBufferData, GL_ARRAY_BUFFER, vertices->count * sizeof(M_VERTEX),
        Vector_GetData(vertices), GL_STATIC_DRAW);

    Vector_Free(vertices);
}

static void M_RenderBegin(const SCENE_SOURCE *const source)
{
    M_PRIV *const p = &m_Priv;
    p->staged = false;
}

static void M_RenderPass(
    const SCENE_SOURCE *const source, const SCENE_PASS pass)
{
    M_PRIV *const p = &m_Priv;
    if (pass != SCENE_PASS_BACKGROUND) {
        return;
    }

    glBindVertexArray(p->vao);
    glBindBuffer(GL_ARRAY_BUFFER, p->vbo);
    glVertexAttrib3f(OUTPUT_MESH_ATTR_NORMAL, 0.0f, 0.0f, 0.0f);
    glVertexAttrib4f(OUTPUT_MESH_ATTR_TEXTURE_SIZE, 0.0f, 0.0f, 1.0f, 1.0f);
    glVertexAttrib2f(OUTPUT_MESH_ATTR_TRAPEZOID_RATIO, 1.0f, 1.0f);
    glVertexAttribI1ui(
        OUTPUT_MESH_ATTR_FLAGS,
        VERT_NO_LIGHTING | VERT_NO_CAUSTICS | VERT_BACKGROUND);
    glVertexAttrib4f(OUTPUT_MESH_ATTR_COLOR, 1.0f, 1.0f, 1.0f, 1.0f);
    glVertexAttrib1f(OUTPUT_MESH_ATTR_SHADE, SHADE_NEUTRAL);
    Output_Shader_UploadViewModelMatrix(p->shader, &g_IDMatrix);

    glDrawElements(
        GL_TRIANGLES, p->vertex_indices->count, GL_UNSIGNED_INT,
        Vector_GetData(p->vertex_indices));
    GFX_GL_CheckError();
}

static bool M_IsDirty(const SCENE_SOURCE *const source, const SCENE_PASS pass)
{
    const M_PRIV *const p = &m_Priv;
    return pass == SCENE_PASS_BACKGROUND && p->staged;
}

void OutputSource_Background_Init(void)
{
    M_PRIV *const p = &m_Priv;
    p->shader = Output_GetMeshShader();
    p->vertex_indices = Vector_Create(sizeof(int32_t));
    p->source.render_begin = M_RenderBegin;
    p->source.render_pass = M_RenderPass;
    p->source.is_dirty = M_IsDirty;
    SceneCompositor_AddSource(&p->source);

    glGenVertexArrays(1, &p->vao);
    glBindVertexArray(p->vao);
    glGenBuffers(1, &p->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, p->vbo);
    glEnableVertexAttribArray(OUTPUT_MESH_ATTR_POS);
    glEnableVertexAttribArray(OUTPUT_MESH_ATTR_UVW);
    glDisableVertexAttribArray(OUTPUT_MESH_ATTR_NORMAL);
    glDisableVertexAttribArray(OUTPUT_MESH_ATTR_TEXTURE_SIZE);
    glDisableVertexAttribArray(OUTPUT_MESH_ATTR_TRAPEZOID_RATIO);
    glDisableVertexAttribArray(OUTPUT_MESH_ATTR_FLAGS);
    glDisableVertexAttribArray(OUTPUT_MESH_ATTR_COLOR);
    glDisableVertexAttribArray(OUTPUT_MESH_ATTR_SHADE);
    glVertexAttribPointer(
        OUTPUT_MESH_ATTR_POS, 3, GL_FLOAT, GL_FALSE, sizeof(M_VERTEX),
        (void *)(intptr_t)offsetof(M_VERTEX, pos));
    glVertexAttribPointer(
        OUTPUT_MESH_ATTR_UVW, 3, GL_FLOAT, GL_FALSE, sizeof(M_VERTEX),
        (void *)(intptr_t)offsetof(M_VERTEX, uvw));
}

void OutputSource_Background_Shutdown(void)
{
    M_PRIV *const p = &m_Priv;
    if (p->vertex_indices != nullptr) {
        Vector_Free(p->vertex_indices);
        p->vertex_indices = nullptr;
    }

    if (p->vao != 0) {
        glDeleteVertexArrays(1, &p->vao);
        p->vao = 0;
    }
    if (p->vbo != 0) {
        glDeleteBuffers(1, &p->vbo);
        p->vbo = 0;
    }
}

void OutputSource_Background_PrepareImage(void)
{
    M_PRIV *const p = &m_Priv;
    M_PrepareVertexGrid(p, 1, 1);
}

void OutputSource_Background_PrepareObject(void)
{
    // TODO: implement me for TR2
}

void OutputSource_Background_Stage(void)
{
    M_PRIV *const p = &m_Priv;
    p->staged = true;
}
