#include <trx/game/output/sources/sky.h>

#include <trx/config.h>
#include <trx/core/utils.h>
#include <trx/game/output.h>
#include <trx/game/output/scene_compositor.h>
#include <trx/game/output/utils.h>
#include <trx/gl/sampler.h>
#include <trx/gl/utils.h>

// TR4 flat sky layers: for each layer, two colored quads tiling along the
// X axis, positioned by a model matrix that follows the camera and applies
// the scroll offset (see OG's DrawFlatSky).

#define M_LAYER_HALF_X 5632
#define M_LAYER_HALF_Z 4864
#define M_QUAD_OFFSET_X (-11264)
#define M_MAX_LAYERS 2
#define M_GRADIENT_MAX_QUADS 16
// Pull the overlay slightly towards the camera so it doesn't z-fight the
// coplanar skybox faces it covers.
#define M_GRADIENT_DEPTH_ADJUSTMENT (-0.005f)

typedef struct {
    XYZW_F pos;
    XYZ_F uvw;
} M_VERTEX;

typedef struct {
    XYZW_F pos;
    RGBA_F color;
} M_GRADIENT_VERTEX;

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
    TRX_GL_SAMPLER sampler;
    MATRIX gradient_wmatrix;
    int32_t gradient_vertex_count;
    bool gradient_staged;
    const OBJECT_MESH *gradient_mesh;
    RGBA_F gradient_color;
    GLuint gradient_vao;
    GLuint gradient_vbo;
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
    p->gradient_staged = false;
}

// Flat fog-colored quads alpha-blended over the horizon mesh bottom edge
// (see OutputSource_Sky_StageFogGradient).
static void M_RenderFogGradient(M_PRIV *const p)
{
    glBindVertexArray(p->gradient_vao);
    glVertexAttrib4f(OUTPUT_MESH_ATTR_NORMAL, 0.0f, 0.0f, 0.0f, 0.0f);
    glVertexAttrib4f(OUTPUT_MESH_ATTR_TEXTURE_SIZE, 0.0f, 0.0f, 1.0f, 1.0f);
    glVertexAttrib2f(OUTPUT_MESH_ATTR_TRAPEZOID_RATIO, 1.0f, 1.0f);
    glVertexAttrib1f(OUTPUT_MESH_ATTR_SHADE, SHADE_NEUTRAL);
    glVertexAttribI1ui(
        OUTPUT_MESH_ATTR_FLAGS,
        VERT_FLAT_SHADED | VERT_NO_LIGHTING | VERT_NO_WIBBLE
            | VERT_NO_ALPHA_DISCARD);
    Output_MeshShader_UploadTint(p->shader, COLOR_RGBA_F_WHITE);
    Output_MeshShader_UploadModelMatrix(p->shader, &p->gradient_wmatrix);
    // Like OG, the skybox is drawn without backface culling.
    glDisable(GL_CULL_FACE);
    glDrawArrays(GL_TRIANGLES, 0, p->gradient_vertex_count);
    glEnable(GL_CULL_FACE);
    TRX_GL_CheckError();
}

static void M_RenderPass(
    const SCENE_SOURCE *const source, const SCENE_PASS pass)
{
    M_PRIV *const p = &m_Priv;
    if (pass == SCENE_PASS_TRANSPARENT) {
        if (p->gradient_staged) {
            M_RenderFogGradient(p);
        }
        return;
    }
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
    // back to flat-shaded colored quads. The layer color is a light value, so
    // the quads suppress fog without suppressing the overbright lighting that
    // carries most of the sky hue.
    uint32_t flags = VERT_NO_FOG | VERT_NO_WIBBLE | VERT_NO_ALPHA_DISCARD;
    if (texture_page >= 0) {
        glEnableVertexAttribArray(OUTPUT_MESH_ATTR_UVW);
        // Layer colors are in the OG 128-neutral scale; the shader doubles
        // them and adds the overbright excess after texturing.
        flags |= VERT_OVERBRIGHT | VERT_TEX_WRAP;
    } else {
        glDisableVertexAttribArray(OUTPUT_MESH_ATTR_UVW);
        glVertexAttrib3f(OUTPUT_MESH_ATTR_UVW, 0.0f, 0.0f, 0.0f);
        flags |= VERT_FLAT_SHADED;
    }
    glVertexAttribI1ui(OUTPUT_MESH_ATTR_FLAGS, flags);
    Output_MeshShader_UploadTint(p->shader, COLOR_RGBA_F_WHITE);

    GLint prev_sampler = 0;
    if (texture_page >= 0) {
        const GLint gl_filter =
            g_Config.rendering.texture_filter == TEXTURE_FILTER_BILINEAR
            ? GL_LINEAR
            : GL_NEAREST;
        TRX_GL_Sampler_Parameteri(
            &p->sampler, GL_TEXTURE_MIN_FILTER, gl_filter);
        TRX_GL_Sampler_Parameteri(
            &p->sampler, GL_TEXTURE_MAG_FILTER, gl_filter);
        glGetIntegeri_v(GL_SAMPLER_BINDING, 0, &prev_sampler);
        TRX_GL_Sampler_Bind(&p->sampler, 0);
    }

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
    if (texture_page >= 0) {
        glBindSampler(0, (GLuint)prev_sampler);
    }
    TRX_GL_CheckError();
}

static bool M_IsDirty(const SCENE_SOURCE *const source, const SCENE_PASS pass)
{
    const M_PRIV *const p = &m_Priv;
    return (pass == SCENE_PASS_BACKGROUND && p->layer_count > 0)
        || (pass == SCENE_PASS_TRANSPARENT && p->gradient_staged);
}

// The gradient geometry only depends on the mesh and the fog color, both
// fixed for the level's duration - rebuild the buffer only when they change.
static void M_UploadFogGradient(
    M_PRIV *const p, const OBJECT_MESH *const mesh, const RGBA_F color)
{
    M_GRADIENT_VERTEX vertices[M_GRADIENT_MAX_QUADS * OUTPUT_QUAD_VERTICES];
    const int32_t quad_count =
        MIN(mesh->tex_face4s.count, M_GRADIENT_MAX_QUADS);
    int32_t v = 0;
    for (int32_t i = 0; i < quad_count; i++) {
        const FACE *const face = &mesh->tex_face4s.data[i];
        for (int32_t j = 0; j < OUTPUT_QUAD_VERTICES; j++) {
            const int32_t k = OUTPUT_QUAD_TO_FAN(j);
            const XYZ_16 pos = mesh->vertices[face->vertices[k]];
            // OG paints half fog on each quad's first two vertices and full
            // fog on the last two, at the mesh's bottom edge. The overlay
            // starts from zero instead, so the gradient fades in rather than
            // meeting the unfogged sky above it at a seam.
            const float fog = k < 2 ? 0.0f : 1.0f;
            vertices[v++] = (M_GRADIENT_VERTEX) {
                .pos = {
                    .x = pos.x,
                    .y = pos.y,
                    .z = pos.z,
                    .w = M_GRADIENT_DEPTH_ADJUSTMENT,
                },
                .color = {
                    .r = color.r,
                    .g = color.g,
                    .b = color.b,
                    .a = color.a * fog,
                },
            };
        }
    }
    glBindBuffer(GL_ARRAY_BUFFER, p->gradient_vbo);
    TRX_GL_TRACK_DATA(
        glBufferData, GL_ARRAY_BUFFER, v * sizeof(M_GRADIENT_VERTEX), vertices,
        GL_STATIC_DRAW);
    p->gradient_mesh = mesh;
    p->gradient_color = color;
    p->gradient_vertex_count = v;
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

    TRX_GL_Sampler_Init(&p->sampler);
    TRX_GL_Sampler_Parameteri(&p->sampler, GL_TEXTURE_WRAP_S, GL_REPEAT);
    TRX_GL_Sampler_Parameteri(&p->sampler, GL_TEXTURE_WRAP_T, GL_REPEAT);

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

    glGenVertexArrays(1, &p->gradient_vao);
    glBindVertexArray(p->gradient_vao);
    glGenBuffers(1, &p->gradient_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, p->gradient_vbo);
    glEnableVertexAttribArray(OUTPUT_MESH_ATTR_POS);
    glEnableVertexAttribArray(OUTPUT_MESH_ATTR_COLOR);
    glDisableVertexAttribArray(OUTPUT_MESH_ATTR_NORMAL);
    glDisableVertexAttribArray(OUTPUT_MESH_ATTR_UVW);
    glDisableVertexAttribArray(OUTPUT_MESH_ATTR_TEXTURE_SIZE);
    glDisableVertexAttribArray(OUTPUT_MESH_ATTR_TRAPEZOID_RATIO);
    glDisableVertexAttribArray(OUTPUT_MESH_ATTR_FLAGS);
    glDisableVertexAttribArray(OUTPUT_MESH_ATTR_SHADE);
    glVertexAttribPointer(
        OUTPUT_MESH_ATTR_POS, 4, GL_FLOAT, GL_FALSE, sizeof(M_GRADIENT_VERTEX),
        (void *)(intptr_t)offsetof(M_GRADIENT_VERTEX, pos));
    glVertexAttribPointer(
        OUTPUT_MESH_ATTR_COLOR, 4, GL_FLOAT, GL_FALSE,
        sizeof(M_GRADIENT_VERTEX),
        (void *)(intptr_t)offsetof(M_GRADIENT_VERTEX, color));
}

void OutputSource_Sky_Shutdown(void)
{
    M_PRIV *const p = &m_Priv;
    TRX_GL_Sampler_Close(&p->sampler);
    if (p->vao != 0) {
        glDeleteVertexArrays(1, &p->vao);
        p->vao = 0;
    }
    if (p->vbo != 0) {
        glDeleteBuffers(1, &p->vbo);
        p->vbo = 0;
    }
    if (p->gradient_vao != 0) {
        glDeleteVertexArrays(1, &p->gradient_vao);
        p->gradient_vao = 0;
    }
    if (p->gradient_vbo != 0) {
        glDeleteBuffers(1, &p->gradient_vbo);
        p->gradient_vbo = 0;
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

void OutputSource_Sky_InvalidateFogGradient(void)
{
    M_PRIV *const p = &m_Priv;
    p->gradient_mesh = nullptr;
    p->gradient_vertex_count = 0;
    p->gradient_staged = false;
}

void OutputSource_Sky_StageFogGradient(
    const MATRIX *const wmatrix, const OBJECT_MESH *const mesh,
    const RGBA_F color)
{
    M_PRIV *const p = &m_Priv;
    if (mesh != p->gradient_mesh || color.r != p->gradient_color.r
        || color.g != p->gradient_color.g || color.b != p->gradient_color.b
        || color.a != p->gradient_color.a) {
        M_UploadFogGradient(p, mesh, color);
    }
    if (p->gradient_vertex_count == 0) {
        return;
    }
    p->gradient_wmatrix = *wmatrix;
    p->gradient_staged = true;
}
