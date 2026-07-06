#include <trx/game/output/sources/sky.h>

#include <trx/game/output.h>
#include <trx/game/output/scene_compositor.h>
#include <trx/game/output/utils.h>
#include <trx/gl/utils.h>

// TR4 flat sky layers: for each layer, two colored quads tiling along the
// X axis, positioned by a model matrix that follows the camera and applies
// the scroll offset (see OG's DrawFlatSky).

#define M_LAYER_HALF_X 5632
#define M_LAYER_HALF_Z 4864
#define M_QUAD_OFFSET_X (-11264)
#define M_MAX_LAYERS 2

typedef struct {
    XYZW_F pos;
    XYZ_F uvw;
} M_VERTEX;

typedef struct {
    MATRIX wmatrix;
    RGB_888 color; // OG 128-neutral scale
    bool additive;
} M_LAYER;

typedef struct {
    SCENE_SOURCE source;
    OUTPUT_MESH_SHADER *shader;
    M_LAYER layers[M_MAX_LAYERS];
    int32_t layer_count;
    int32_t texture_page;
    GLuint vao;
    GLuint vbo;
} M_PRIV;

static M_PRIV m_Priv;

static void M_UploadVertices(M_PRIV *const p)
{
    // The sky image spans each quad in full, tiling along the X axis.
    // OG's DrawFlatSky passes its corners to AddQuadSorted in reverse order
    // (3,2,1,0), and AddQuadSorted binds u1/v1 to the first listed vertex -
    // so UV (0,0) lands on the (-X,-Z) corner, with V growing towards +Z.
    M_VERTEX vertices[12];
    int32_t v = 0;
    for (int32_t quad = 0; quad < 2; quad++) {
        const float dx = quad * M_QUAD_OFFSET_X;
        const M_VERTEX corners[4] = {
            {
                .pos = { .x = dx - M_LAYER_HALF_X, .z = -M_LAYER_HALF_Z },
                .uvw = { .x = 0.0f, .y = 0.0f, .z = p->texture_page },
            },
            {
                .pos = { .x = dx + M_LAYER_HALF_X, .z = -M_LAYER_HALF_Z },
                .uvw = { .x = 1.0f, .y = 0.0f, .z = p->texture_page },
            },
            {
                .pos = { .x = dx + M_LAYER_HALF_X, .z = M_LAYER_HALF_Z },
                .uvw = { .x = 1.0f, .y = 1.0f, .z = p->texture_page },
            },
            {
                .pos = { .x = dx - M_LAYER_HALF_X, .z = M_LAYER_HALF_Z },
                .uvw = { .x = 0.0f, .y = 1.0f, .z = p->texture_page },
            },
        };
        for (int32_t i = 0; i < OUTPUT_QUAD_VERTICES; i++) {
            vertices[v++] = corners[OUTPUT_QUAD_TO_FAN(i)];
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, p->vbo);
    TRX_GL_TRACK_DATA(
        glBufferData, GL_ARRAY_BUFFER, sizeof(vertices), vertices,
        GL_STATIC_DRAW);
}

static void M_RenderBegin(const SCENE_SOURCE *const source)
{
    M_PRIV *const p = &m_Priv;
    p->layer_count = 0;
}

static void M_RenderPass(
    const SCENE_SOURCE *const source, const SCENE_PASS pass)
{
    M_PRIV *const p = &m_Priv;
    if (pass != SCENE_PASS_BACKGROUND) {
        return;
    }

    const int32_t texture_page = Output_Sky_GetTexturePage();
    if (texture_page != p->texture_page) {
        p->texture_page = texture_page;
        M_UploadVertices(p);
    }

    glBindVertexArray(p->vao);
    glBindBuffer(GL_ARRAY_BUFFER, p->vbo);
    glVertexAttrib4f(OUTPUT_MESH_ATTR_NORMAL, 0.0f, 0.0f, 0.0f, 0.0f);
    glVertexAttrib4f(OUTPUT_MESH_ATTR_TEXTURE_SIZE, 0.0f, 0.0f, 1.0f, 1.0f);
    glVertexAttrib2f(OUTPUT_MESH_ATTR_TRAPEZOID_RATIO, 1.0f, 1.0f);
    glVertexAttrib1f(OUTPUT_MESH_ATTR_SHADE, SHADE_NEUTRAL);

    // With a sky image, tint it with the layer color; without one, fall
    // back to flat-shaded colored quads.
    uint32_t flags = VERT_NO_LIGHTING | VERT_NO_WIBBLE | VERT_NO_ALPHA_DISCARD;
    if (texture_page >= 0) {
        glEnableVertexAttribArray(OUTPUT_MESH_ATTR_UVW);
        // Layer colors are in the OG 128-neutral scale; the shader doubles
        // them and adds the overbright excess after texturing.
        flags |= VERT_OVERBRIGHT;
    } else {
        glDisableVertexAttribArray(OUTPUT_MESH_ATTR_UVW);
        glVertexAttrib3f(OUTPUT_MESH_ATTR_UVW, 0.0f, 0.0f, 0.0f);
        flags |= VERT_FLAT_SHADED;
    }
    glVertexAttribI1ui(OUTPUT_MESH_ATTR_FLAGS, flags);
    Output_MeshShader_UploadTint(p->shader, (RGB_F) { 1.0f, 1.0f, 1.0f });

    for (int32_t i = 0; i < p->layer_count; i++) {
        const M_LAYER *const layer = &p->layers[i];
        RGB_888 color = layer->color;
        if (texture_page < 0) {
            // The flat-shaded fallback bypasses the overbright path, so
            // apply the neutral-scale doubling on the CPU.
            color.r = MIN(color.r * 2, 255);
            color.g = MIN(color.g * 2, 255);
            color.b = MIN(color.b * 2, 255);
        }
        if (layer->additive) {
            glBlendFunc(GL_ONE, GL_ONE);
        }
        glVertexAttrib4f(
            OUTPUT_MESH_ATTR_COLOR, color.r / 255.0f, color.g / 255.0f,
            color.b / 255.0f, 1.0f);
        Output_MeshShader_UploadModelMatrix(p->shader, &layer->wmatrix);
        glDrawArrays(GL_TRIANGLES, 0, 12);
        if (layer->additive) {
            glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        }
    }
    TRX_GL_CheckError();
}

static bool M_IsDirty(const SCENE_SOURCE *const source, const SCENE_PASS pass)
{
    const M_PRIV *const p = &m_Priv;
    return pass == SCENE_PASS_BACKGROUND && p->layer_count > 0;
}

void OutputSource_Sky_Init(void)
{
    M_PRIV *const p = &m_Priv;
    p->shader = Output_GetMeshShader();
    p->source.render_begin = M_RenderBegin;
    p->source.render_pass = M_RenderPass;
    p->source.is_dirty = M_IsDirty;
    p->texture_page = -1;
    SceneCompositor_AddSource(&p->source);

    glGenVertexArrays(1, &p->vao);
    glBindVertexArray(p->vao);

    glGenBuffers(1, &p->vbo);
    M_UploadVertices(p);

    glEnableVertexAttribArray(OUTPUT_MESH_ATTR_POS);
    glDisableVertexAttribArray(OUTPUT_MESH_ATTR_NORMAL);
    glDisableVertexAttribArray(OUTPUT_MESH_ATTR_UVW);
    glDisableVertexAttribArray(OUTPUT_MESH_ATTR_TEXTURE_SIZE);
    glDisableVertexAttribArray(OUTPUT_MESH_ATTR_TRAPEZOID_RATIO);
    glDisableVertexAttribArray(OUTPUT_MESH_ATTR_FLAGS);
    glDisableVertexAttribArray(OUTPUT_MESH_ATTR_COLOR);
    glDisableVertexAttribArray(OUTPUT_MESH_ATTR_SHADE);
    glVertexAttribPointer(
        OUTPUT_MESH_ATTR_POS, 4, GL_FLOAT, GL_FALSE, sizeof(M_VERTEX),
        (void *)(intptr_t)offsetof(M_VERTEX, pos));
    glVertexAttribPointer(
        OUTPUT_MESH_ATTR_UVW, 3, GL_FLOAT, GL_FALSE, sizeof(M_VERTEX),
        (void *)(intptr_t)offsetof(M_VERTEX, uvw));
}

void OutputSource_Sky_Shutdown(void)
{
    M_PRIV *const p = &m_Priv;
    if (p->vao != 0) {
        glDeleteVertexArrays(1, &p->vao);
        p->vao = 0;
    }
    if (p->vbo != 0) {
        glDeleteBuffers(1, &p->vbo);
        p->vbo = 0;
    }
}

void OutputSource_Sky_StageLayer(
    const MATRIX *const wmatrix, const RGB_888 color, const bool additive)
{
    M_PRIV *const p = &m_Priv;
    if (p->layer_count >= M_MAX_LAYERS) {
        return;
    }
    p->layers[p->layer_count++] = (M_LAYER) {
        .wmatrix = *wmatrix,
        .color = color,
        .additive = additive,
    };
}
