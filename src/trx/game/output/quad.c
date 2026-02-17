#include <trx/game/output/quad.h>

#include <trx/config.h>
#include <trx/debug.h>
#include <trx/game/output.h>
#include <trx/gl/gl/utils.h>
#include <trx/memory.h>
#include <trx/utils.h>

#include <GL/glew.h>
#include <string.h>

typedef enum {
    M_UNIFORM_TEXTURE_MAIN,
    M_UNIFORM_TEXTURE_SIZE,
    M_UNIFORM_EFFECT,
    M_UNIFORM_OPACITY,
    M_UNIFORM_BRIGHTNESS_SCALE,
    M_UNIFORM_FIT_MODE,
    M_UNIFORM_SRC_ASPECT,
    M_UNIFORM_NUMBER_OF,
} M_UNIFORM;

typedef struct {
    struct {
        GLfloat x;
        GLfloat y;
    } pos;
    struct {
        GLfloat u;
        GLfloat v;
    } uv;
} M_VERTEX;

struct OUTPUT_QUAD {
    TRX_GL_VERTEX_ARRAY vertex_format;
    TRX_GL_BUFFER surface_buffer;
    TRX_GL_TEXTURE surface_texture;
    TRX_GL_PROGRAM program;

    M_VERTEX *vertices;
    int32_t vertex_count;

    bool ready;
    TRX_GL_2D_SURFACE_DESC desc;
    struct {
        int32_t x;
        int32_t y;
    } repeat;

    OUTPUT_QUAD_EFFECT effect;

    float opacity;
    float brightness_scale;

    OUTPUT_QUAD_FIT_MODE fit_mode;
    float src_aspect;

    bool use_external_texture;
    GLuint external_texture_id;

    // shader variable locations
    GLint loc[M_UNIFORM_NUMBER_OF];
} M_PRIV;

static const M_VERTEX m_Vertices[] = {
    { .pos = { .x = 0.0, .y = 0.0 }, .uv = { .u = 0.0, .v = 0.0 } },
    { .pos = { .x = 1.0, .y = 0.0 }, .uv = { .u = 1.0, .v = 0.0 } },
    { .pos = { .x = 0.0, .y = 1.0 }, .uv = { .u = 0.0, .v = 1.0 } },
    { .pos = { .x = 0.0, .y = 1.0 }, .uv = { .u = 0.0, .v = 1.0 } },
    { .pos = { .x = 1.0, .y = 0.0 }, .uv = { .u = 1.0, .v = 0.0 } },
    { .pos = { .x = 1.0, .y = 1.0 }, .uv = { .u = 1.0, .v = 1.0 } },
};

static void M_UploadVertices(OUTPUT_QUAD *const r)
{
    if (!r->ready) {
        return;
    }

    const int32_t mapping[] = { 0, 1, 3, 3, 1, 2 };
    r->vertex_count = r->repeat.x * r->repeat.y * 6;
    r->vertices = Memory_Realloc(
        r->vertices, r->repeat.x * r->repeat.y * 6 * sizeof(M_VERTEX));

    M_VERTEX *ptr = r->vertices;
    for (int32_t y = 0; y < r->repeat.y; y++) {
        for (int32_t x = 0; x < r->repeat.x; x++) {
            for (int32_t i = 0; i < 6; i++) {
                const float x_factor = (float)x / (float)r->repeat.x;
                const float y_factor = (float)y / (float)r->repeat.y;
                const float x_offset = 1.0f / (float)r->repeat.x;
                const float y_offset = 1.0f / (float)r->repeat.y;

                ptr->pos.x = m_Vertices[i].pos.x * x_offset + x_factor;
                ptr->pos.y = m_Vertices[i].pos.y * y_offset + y_factor;
                ptr->uv.u = r->desc.uv[mapping[i]].u;
                ptr->uv.v = r->desc.uv[mapping[i]].v;

                ptr++;
            }
        }
    }
    TRX_GL_Buffer_Bind(&r->surface_buffer);
    TRX_GL_Buffer_Data(
        &r->surface_buffer, sizeof(M_VERTEX) * 6 * r->repeat.x * r->repeat.y,
        r->vertices, GL_STATIC_DRAW);
}

OUTPUT_QUAD *Output_Quad_Create(void)
{
    OUTPUT_QUAD *const r = Memory_Alloc(sizeof(OUTPUT_QUAD));

    r->effect = OUTPUT_QUAD_EFFECT_NONE;
    r->opacity = 1.0f;
    r->brightness_scale = 1.0f;
    r->repeat.x = 1;
    r->repeat.y = 1;

    r->fit_mode = OUTPUT_QUAD_FIT_STRETCH;
    r->src_aspect = 1.0f;

    r->vertices = nullptr;
    r->vertex_count = 6;
    r->vertex_format.initialized = false;
    r->use_external_texture = false;
    r->external_texture_id = 0;

    TRX_GL_Buffer_Init(&r->surface_buffer, GL_ARRAY_BUFFER);
    TRX_GL_Buffer_Bind(&r->surface_buffer);
    TRX_GL_Buffer_Data(
        &r->surface_buffer, sizeof(m_Vertices), m_Vertices, GL_STATIC_DRAW);

    TRX_GL_VertexArray_Init(&r->vertex_format);
    TRX_GL_VertexArray_Bind(&r->vertex_format);
    TRX_GL_VertexArray_Attribute(
        &r->vertex_format, 0, 2, GL_FLOAT, GL_FALSE, sizeof(M_VERTEX),
        offsetof(M_VERTEX, pos));
    TRX_GL_VertexArray_Attribute(
        &r->vertex_format, 1, 2, GL_FLOAT, GL_FALSE, sizeof(M_VERTEX),
        offsetof(M_VERTEX, uv));
    TRX_GL_CheckError();

    TRX_GL_Texture_Init(&r->surface_texture, GL_TEXTURE_2D);

    TRX_GL_Program_Init(&r->program);
    TRX_GL_Program_AttachShader(&r->program, GL_VERTEX_SHADER, "2d.glsl");
    TRX_GL_Program_AttachShader(&r->program, GL_FRAGMENT_SHADER, "2d.glsl");
    TRX_GL_Program_FragmentData(&r->program, "outColor");
    TRX_GL_Program_Link(&r->program);

    struct {
        M_UNIFORM loc;
        const char *name;
    } uniforms[] = {
        { M_UNIFORM_TEXTURE_MAIN, "uTexMain" },
        { M_UNIFORM_TEXTURE_SIZE, "uTexSize" },
        { M_UNIFORM_EFFECT, "uEffect" },
        { M_UNIFORM_OPACITY, "uOpacity" },
        { M_UNIFORM_BRIGHTNESS_SCALE, "uBrightnessScale" },
        { M_UNIFORM_FIT_MODE, "uFitMode" },
        { M_UNIFORM_SRC_ASPECT, "uSrcAspect" },
        { -1, nullptr },
    };
    for (int32_t i = 0; uniforms[i].name != nullptr; i++) {
        r->loc[uniforms[i].loc] =
            TRX_GL_Program_UniformLocation(&r->program, uniforms[i].name);
        TRX_GL_CheckError();
    }

    TRX_GL_Program_Bind(&r->program);
    TRX_GL_Program_Uniform1i(&r->program, r->loc[M_UNIFORM_TEXTURE_MAIN], 0);
    TRX_GL_Program_Uniform4f(
        &r->program, r->loc[M_UNIFORM_TEXTURE_SIZE], 0.0f, 0.0f, 1.0f, 1.0f);
    TRX_GL_Program_Uniform1i(&r->program, r->loc[M_UNIFORM_EFFECT], r->effect);
    TRX_GL_Program_Uniform1f(
        &r->program, r->loc[M_UNIFORM_OPACITY], r->opacity);
    TRX_GL_Program_Uniform1f(
        &r->program, r->loc[M_UNIFORM_BRIGHTNESS_SCALE], r->brightness_scale);
    TRX_GL_Program_Uniform1i(
        &r->program, r->loc[M_UNIFORM_FIT_MODE], (int32_t)r->fit_mode);
    TRX_GL_Program_Uniform1f(
        &r->program, r->loc[M_UNIFORM_SRC_ASPECT], r->src_aspect);
    TRX_GL_CheckError();

    return r;
}

void Output_Quad_Destroy(OUTPUT_QUAD *const r)
{
    ASSERT(r != nullptr);

    TRX_GL_VertexArray_Close(&r->vertex_format);
    TRX_GL_Buffer_Close(&r->surface_buffer);
    TRX_GL_Texture_Close(&r->surface_texture);
    TRX_GL_Program_Close(&r->program);
    Memory_FreePointer(&r->vertices);
    Memory_Free(r);
}

void Output_Quad_Upload(
    OUTPUT_QUAD *const r, TRX_GL_2D_SURFACE_DESC *const desc,
    const uint8_t *const data)
{
    ASSERT(r != nullptr);

    bool reupload_vert = false;
    if (memcmp(r->desc.uv, desc->uv, sizeof(desc->uv)) != 0) {
        reupload_vert = true;
    }
    if (!r->ready) {
        reupload_vert = true;
    }

    glActiveTexture(GL_TEXTURE0);
    TRX_GL_Texture_Bind(&r->surface_texture);

    // update buffer if the size is unchanged, otherwise create a new one
    if (r->desc.width != desc->width || r->desc.height != desc->height
        || r->desc.tex_format != desc->tex_format
        || r->desc.tex_type != desc->tex_type) {
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        TRX_GL_CheckError();
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        TRX_GL_CheckError();
        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_RGBA, desc->width, desc->height, 0,
            desc->tex_format, desc->tex_type, data);
        TRX_GL_CheckError();
    } else {
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        TRX_GL_CheckError();
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        TRX_GL_CheckError();
        glTexSubImage2D(
            GL_TEXTURE_2D, 0, 0, 0, desc->width, desc->height, desc->tex_format,
            desc->tex_type, data);
        TRX_GL_CheckError();
    }

    r->ready = true;
    r->desc = *desc;
    r->use_external_texture = false;
    r->external_texture_id = 0;
    if (reupload_vert) {
        M_UploadVertices(r);
    }
}

void Output_Quad_SetExternalTexture(
    OUTPUT_QUAD *const r, const GLuint texture_id, const int32_t width,
    const int32_t height, const bool flip_y)
{
    ASSERT(r != nullptr);
    r->use_external_texture = true;
    r->external_texture_id = texture_id;

    const float v0 = flip_y ? 1.0f : 0.0f;
    const float v1 = flip_y ? 0.0f : 1.0f;
    TRX_GL_2D_SURFACE_DESC desc = {
        .width = width,
        .height = height,
        .bit_count = 32,
        .tex_format = GL_RGBA,
        .tex_type = GL_UNSIGNED_INT_8_8_8_8_REV,
        .uv = {
            { 0.0f, v0 },
            { 1.0f, v0 },
            { 1.0f, v1 },
            { 0.0f, v1 },
        },
        .pitch = width * 4,
    };

    const bool reupload_vert =
        memcmp(r->desc.uv, desc.uv, sizeof(desc.uv)) != 0 || !r->ready;
    r->ready = true;
    r->desc = desc;
    if (reupload_vert) {
        M_UploadVertices(r);
    }
}

void Output_Quad_ClearExternalTexture(OUTPUT_QUAD *const r)
{
    ASSERT(r != nullptr);
    r->use_external_texture = false;
    r->external_texture_id = 0;
}

void Output_Quad_SetTextureSize(
    OUTPUT_QUAD *const r, const OUTPUT_QUAD_TEXTURE_SIZE *const size)
{
    ASSERT(r != nullptr);
    TRX_GL_Program_Bind(&r->program);
    if (size == nullptr) {
        TRX_GL_Program_Uniform4f(
            &r->program, r->loc[M_UNIFORM_TEXTURE_SIZE], 0.0f, 0.0f, 1.0f,
            1.0f);
    } else {
        TRX_GL_Program_Uniform4f(
            &r->program, r->loc[M_UNIFORM_TEXTURE_SIZE], size->x0, size->y0,
            size->x1, size->y1);
    }
}

void Output_Quad_SetRepeat(
    OUTPUT_QUAD *const r, const int32_t x, const int32_t y)
{
    ASSERT(r != nullptr);
    if (r->repeat.x == x && r->repeat.y == y) {
        return;
    }
    r->repeat.x = x;
    r->repeat.y = y;
    M_UploadVertices(r);
}

void Output_Quad_SetEffect(OUTPUT_QUAD *const r, const uint32_t effect)
{
    ASSERT(r != nullptr);

    if (r->effect != effect) {
        TRX_GL_Program_Bind(&r->program);
        TRX_GL_Program_Uniform1i(&r->program, r->loc[M_UNIFORM_EFFECT], effect);
        r->effect = effect;
    }
}

void Output_Quad_SetOpacity(OUTPUT_QUAD *const r, const float opacity)
{
    ASSERT(r != nullptr);

    if (r->opacity != opacity) {
        TRX_GL_Program_Bind(&r->program);
        TRX_GL_Program_Uniform1f(
            &r->program, r->loc[M_UNIFORM_OPACITY], opacity);
        r->opacity = opacity;
    }
}

void Output_Quad_SetBrightnessScale(
    OUTPUT_QUAD *const r, const float brightness_scale)
{
    ASSERT(r != nullptr);

    if (r->brightness_scale != brightness_scale) {
        TRX_GL_Program_Bind(&r->program);
        TRX_GL_Program_Uniform1f(
            &r->program, r->loc[M_UNIFORM_BRIGHTNESS_SCALE], brightness_scale);
        r->brightness_scale = brightness_scale;
    }
}

void Output_Quad_SetFit(
    OUTPUT_QUAD *const r, const OUTPUT_QUAD_FIT_MODE fit_mode,
    const float src_w, const float src_h)
{
    ASSERT(r != nullptr);

    if (src_w <= 0.0f || src_h <= 0.0f) {
        Output_Quad_ClearFit(r);
        return;
    }
    const float src_aspect = src_w / src_h;
    if (r->fit_mode == fit_mode && r->src_aspect == src_aspect) {
        return;
    }

    r->fit_mode = fit_mode;
    r->src_aspect = src_aspect;

    TRX_GL_Program_Bind(&r->program);
    TRX_GL_Program_Uniform1i(
        &r->program, r->loc[M_UNIFORM_FIT_MODE], (int32_t)fit_mode);
    TRX_GL_Program_Uniform1f(
        &r->program, r->loc[M_UNIFORM_SRC_ASPECT], src_aspect);
}

void Output_Quad_ClearFit(OUTPUT_QUAD *const r)
{
    ASSERT(r != nullptr);
    if (r->fit_mode == OUTPUT_QUAD_FIT_STRETCH && r->src_aspect == 1.0f) {
        return;
    }

    r->fit_mode = OUTPUT_QUAD_FIT_STRETCH;
    r->src_aspect = 1.0f;
    TRX_GL_Program_Bind(&r->program);
    TRX_GL_Program_Uniform1i(
        &r->program, r->loc[M_UNIFORM_FIT_MODE], (int32_t)r->fit_mode);
    TRX_GL_Program_Uniform1f(
        &r->program, r->loc[M_UNIFORM_SRC_ASPECT], r->src_aspect);
}

void Output_Quad_Render(OUTPUT_QUAD *const r)
{
    ASSERT(r != nullptr);

    TRX_GL_Program_Bind(&r->program);

    TRX_GL_Program_Uniform1i(&r->program, r->loc[M_UNIFORM_EFFECT], r->effect);
    TRX_GL_Buffer_Bind(&r->surface_buffer);
    TRX_GL_VertexArray_Bind(&r->vertex_format);

    glActiveTexture(GL_TEXTURE0);
    if (r->use_external_texture) {
        glBindTexture(GL_TEXTURE_2D, r->external_texture_id);
    } else {
        TRX_GL_Texture_Bind(&r->surface_texture);
    }

    const GLboolean was_blend_enabled = glIsEnabled(GL_BLEND);
    if (was_blend_enabled) {
        glDisable(GL_BLEND);
    }

    GLint bound_polygon_mode[2];
    glGetIntegerv(GL_POLYGON_MODE, &bound_polygon_mode[0]);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    const GLboolean was_depth_test_enabled = glIsEnabled(GL_DEPTH_TEST);
    if (was_depth_test_enabled) {
        glDisable(GL_DEPTH_TEST);
    }

    glDrawArrays(GL_TRIANGLES, 0, r->vertex_count);
    glPolygonMode(GL_FRONT_AND_BACK, bound_polygon_mode[0]);
    if (was_depth_test_enabled) {
        glEnable(GL_DEPTH_TEST);
    }
    if (was_blend_enabled) {
        glEnable(GL_BLEND);
    }
    TRX_GL_CheckError();
}

void Output_Quad_RenderWithBlend(OUTPUT_QUAD *const r)
{
    ASSERT(r != nullptr);

    TRX_GL_Program_Bind(&r->program);
    TRX_GL_Program_Uniform1i(&r->program, r->loc[M_UNIFORM_EFFECT], r->effect);
    TRX_GL_Buffer_Bind(&r->surface_buffer);
    TRX_GL_VertexArray_Bind(&r->vertex_format);

    glActiveTexture(GL_TEXTURE0);
    if (r->use_external_texture) {
        glBindTexture(GL_TEXTURE_2D, r->external_texture_id);
    } else {
        TRX_GL_Texture_Bind(&r->surface_texture);
    }

    const GLboolean was_blend_enabled = glIsEnabled(GL_BLEND);
    GLint prev_blend_src = 0;
    GLint prev_blend_dst = 0;
    glGetIntegerv(GL_BLEND_SRC_RGB, &prev_blend_src);
    glGetIntegerv(GL_BLEND_DST_RGB, &prev_blend_dst);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    GLint bound_polygon_mode[2];
    glGetIntegerv(GL_POLYGON_MODE, &bound_polygon_mode[0]);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    const GLboolean was_depth_test_enabled = glIsEnabled(GL_DEPTH_TEST);
    if (was_depth_test_enabled) {
        glDisable(GL_DEPTH_TEST);
    }

    glDrawArrays(GL_TRIANGLES, 0, r->vertex_count);

    glPolygonMode(GL_FRONT_AND_BACK, bound_polygon_mode[0]);
    if (was_depth_test_enabled) {
        glEnable(GL_DEPTH_TEST);
    }
    glBlendFunc(prev_blend_src, prev_blend_dst);
    if (!was_blend_enabled) {
        glDisable(GL_BLEND);
    }
    TRX_GL_CheckError();
}
