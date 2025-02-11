#include "gfx/2d/2d_renderer.h"

#include "debug.h"
#include "gfx/context.h"
#include "gfx/gl/utils.h"
#include "log.h"
#include "memory.h"
#include "utils.h"

#include <GL/glew.h>
#include <string.h>

typedef enum {
    M_UNIFORM_TEXTURE_MAIN,
    M_UNIFORM_TEXTURE_PALETTE,
    M_UNIFORM_TEXTURE_ALPHA,
    M_UNIFORM_PALETTE_ENABLED,
    M_UNIFORM_ALPHA_ENABLED,
    M_UNIFORM_TINT_ENABLED,
    M_UNIFORM_TINT_COLOR,
    M_UNIFORM_EFFECT,
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

struct GFX_2D_RENDERER {
    GFX_GL_VERTEX_ARRAY vertex_format;
    GFX_GL_BUFFER surface_buffer;
    GFX_GL_TEXTURE surface_texture;
    GFX_GL_TEXTURE palette_texture;
    GFX_GL_TEXTURE alpha_texture;
    GFX_GL_PROGRAM program;

    M_VERTEX *vertices;
    int32_t vertex_count;

    GFX_2D_SURFACE_DESC desc;
    GFX_2D_SURFACE_DESC alpha_desc;
    struct {
        int32_t x;
        int32_t y;
    } repeat;

    GFX_COLOR tint_color;
    GFX_2D_EFFECT effect;
    bool use_palette;
    bool use_alpha;

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

static void M_UploadVertices(GFX_2D_RENDERER *const r)
{
    const int32_t mapping[] = { 0, 1, 3, 3, 1, 2 };
    r->vertex_count = r->repeat.x * r->repeat.y * 6;
    r->vertices = Memory_Realloc(
        r->vertices, r->repeat.x * r->repeat.y * 6 * sizeof(M_VERTEX));
    M_VERTEX *ptr = r->vertices;

    for (int32_t y = 0; y < r->repeat.y; y++) {
        for (int32_t x = 0; x < r->repeat.x; x++) {
            for (int32_t i = 0; i < 6; i++) {

                float xFactor = (float)x / (float)r->repeat.x;
                float yFactor = (float)y / (float)r->repeat.y;
                float xOffset = 1.0f / (float)r->repeat.x;
                float yOffset = 1.0f / (float)r->repeat.y;

                ptr->pos.x = m_Vertices[i].pos.x * xOffset + xFactor;
                ptr->pos.y = m_Vertices[i].pos.y * yOffset + yFactor;
                ptr->uv.u = r->desc.uv[mapping[i]].u;
                ptr->uv.v = r->desc.uv[mapping[i]].v;

                ptr++;
            }
        }
    }
    LOG_DEBUG("%d %d", r->repeat.x, r->repeat.y);
    GFX_GL_Buffer_Bind(&r->surface_buffer);
    GFX_GL_Buffer_Data(
        &r->surface_buffer, sizeof(M_VERTEX) * 6 * r->repeat.x * r->repeat.y,
        r->vertices, GL_STATIC_DRAW);
}

GFX_2D_RENDERER *GFX_2D_Renderer_Create(void)
{
    LOG_INFO("");
    GFX_2D_RENDERER *const r = Memory_Alloc(sizeof(GFX_2D_RENDERER));
    const GFX_CONFIG *const config = GFX_Context_GetConfig();

    r->effect = GFX_2D_EFFECT_NONE;
    r->tint_color = (GFX_COLOR) { .r = 255, .g = 255, .b = 255 };
    r->use_palette = false;
    r->use_alpha = false;
    r->repeat.x = 1;
    r->repeat.y = 1;

    r->vertices = nullptr;
    r->vertex_count = 6;
    r->vertex_format.initialized = false;

    GFX_GL_Buffer_Init(&r->surface_buffer, GL_ARRAY_BUFFER);
    GFX_GL_Buffer_Bind(&r->surface_buffer);
    GFX_GL_Buffer_Data(
        &r->surface_buffer, sizeof(m_Vertices), m_Vertices, GL_STATIC_DRAW);

    GFX_GL_VertexArray_Init(&r->vertex_format);
    GFX_GL_VertexArray_Bind(&r->vertex_format);
    GFX_GL_VertexArray_Attribute(
        &r->vertex_format, 0, 2, GL_FLOAT, GL_FALSE, sizeof(M_VERTEX),
        offsetof(M_VERTEX, pos));
    GFX_GL_VertexArray_Attribute(
        &r->vertex_format, 1, 2, GL_FLOAT, GL_FALSE, sizeof(M_VERTEX),
        offsetof(M_VERTEX, uv));
    GFX_GL_CheckError();

    GFX_GL_Texture_Init(&r->surface_texture, GL_TEXTURE_2D);
    GFX_GL_Texture_Init(&r->palette_texture, GL_TEXTURE_1D);
    GFX_GL_Texture_Init(&r->alpha_texture, GL_TEXTURE_2D);

    GFX_GL_Program_Init(&r->program);
    GFX_GL_Program_AttachShader(
        &r->program, GL_VERTEX_SHADER, "shaders/2d.glsl", config->backend);
    GFX_GL_Program_AttachShader(
        &r->program, GL_FRAGMENT_SHADER, "shaders/2d.glsl", config->backend);
    GFX_GL_Program_FragmentData(&r->program, "outColor");
    GFX_GL_Program_Link(&r->program);

    struct {
        M_UNIFORM loc;
        const char *name;
    } uniforms[] = {
        { M_UNIFORM_TEXTURE_MAIN, "texMain" },
        { M_UNIFORM_TEXTURE_PALETTE, "texPalette" },
        { M_UNIFORM_TEXTURE_ALPHA, "texAlpha" },
        { M_UNIFORM_PALETTE_ENABLED, "paletteEnabled" },
        { M_UNIFORM_ALPHA_ENABLED, "alphaEnabled" },
        { M_UNIFORM_TINT_ENABLED, "tintEnabled" },
        { M_UNIFORM_TINT_COLOR, "tintColor" },
        { M_UNIFORM_EFFECT, "effect" },
        { -1, nullptr },
    };
    for (int32_t i = 0; uniforms[i].name != nullptr; i++) {
        r->loc[uniforms[i].loc] =
            GFX_GL_Program_UniformLocation(&r->program, uniforms[i].name);
        GFX_GL_CheckError();
    }

    GFX_GL_Program_Bind(&r->program);
    GFX_GL_Program_Uniform1i(&r->program, r->loc[M_UNIFORM_TEXTURE_MAIN], 0);
    GFX_GL_Program_Uniform1i(&r->program, r->loc[M_UNIFORM_TEXTURE_PALETTE], 1);
    GFX_GL_Program_Uniform1i(&r->program, r->loc[M_UNIFORM_TEXTURE_ALPHA], 2);
    GFX_GL_Program_Uniform1i(
        &r->program, r->loc[M_UNIFORM_PALETTE_ENABLED], r->use_palette);
    GFX_GL_Program_Uniform1i(
        &r->program, r->loc[M_UNIFORM_ALPHA_ENABLED], r->use_alpha);
    GFX_GL_Program_Uniform1i(
        &r->program, r->loc[M_UNIFORM_TINT_ENABLED],
        r->tint_color.r != 255 || r->tint_color.g != 255
            || r->tint_color.b != 255);
    GFX_GL_Program_Uniform1i(&r->program, r->loc[M_UNIFORM_EFFECT], r->effect);
    GFX_GL_CheckError();

    return r;
}

void GFX_2D_Renderer_Destroy(GFX_2D_RENDERER *const r)
{
    ASSERT(r != nullptr);

    GFX_GL_VertexArray_Close(&r->vertex_format);
    GFX_GL_Buffer_Close(&r->surface_buffer);
    GFX_GL_Texture_Close(&r->surface_texture);
    GFX_GL_Texture_Close(&r->palette_texture);
    GFX_GL_Texture_Close(&r->alpha_texture);
    GFX_GL_Program_Close(&r->program);
    Memory_FreePointer(&r->vertices);
    Memory_Free(r);
}

void GFX_2D_Renderer_UploadSurface(
    GFX_2D_RENDERER *const r, GFX_2D_SURFACE *const surface)
{
    GFX_2D_Renderer_Upload(r, &surface->desc, surface->buffer);
}

void GFX_2D_Renderer_UploadAlphaSurface(
    GFX_2D_RENDERER *const r, GFX_2D_SURFACE *const surface)
{
    ASSERT(r != nullptr);

    if (surface == nullptr) {
        if (r->use_alpha) {
            GFX_GL_Program_Bind(&r->program);
            GFX_GL_Program_Uniform1i(
                &r->program, r->loc[M_UNIFORM_ALPHA_ENABLED], false);
        }
        r->use_alpha = false;
        return;
    }

    if (!r->use_alpha) {
        GFX_GL_Program_Bind(&r->program);
        GFX_GL_Program_Uniform1i(
            &r->program, r->loc[M_UNIFORM_ALPHA_ENABLED], true);
        GFX_GL_CheckError();
        r->use_alpha = true;
    }

    glActiveTexture(GL_TEXTURE2);
    GFX_GL_Texture_Bind(&r->alpha_texture);

    // update buffer if the size is unchanged, otherwise create a new one
    if (r->alpha_desc.width != surface->desc.width
        || r->alpha_desc.height != surface->desc.height
        || r->alpha_desc.tex_format != surface->desc.tex_format
        || r->alpha_desc.tex_type != surface->desc.tex_type) {
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        GFX_GL_CheckError();
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        GFX_GL_CheckError();
        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_RGBA, surface->desc.width,
            surface->desc.height, 0, surface->desc.tex_format,
            surface->desc.tex_type, surface->buffer);
        GFX_GL_CheckError();
    } else {
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        GFX_GL_CheckError();
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        GFX_GL_CheckError();
        glTexSubImage2D(
            GL_TEXTURE_2D, 0, 0, 0, surface->desc.width, surface->desc.height,
            surface->desc.tex_format, surface->desc.tex_type, surface->buffer);
        GFX_GL_CheckError();
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    GFX_GL_CheckError();

    r->alpha_desc = surface->desc;
}

void GFX_2D_Renderer_Upload(
    GFX_2D_RENDERER *const r, GFX_2D_SURFACE_DESC *const desc,
    const uint8_t *const data)
{
    ASSERT(r != nullptr);

    bool reupload_vert = false;
    if (memcmp(r->desc.uv, desc->uv, sizeof(desc->uv)) != 0) {
        reupload_vert = true;
    }

    glActiveTexture(GL_TEXTURE0);
    GFX_GL_Texture_Bind(&r->surface_texture);

    // update buffer if the size is unchanged, otherwise create a new one
    if (r->desc.width != desc->width || r->desc.height != desc->height
        || r->desc.tex_format != desc->tex_format
        || r->desc.tex_type != desc->tex_type) {
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        GFX_GL_CheckError();
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        GFX_GL_CheckError();
        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_RGBA, desc->width, desc->height, 0,
            desc->tex_format, desc->tex_type, data);
        GFX_GL_CheckError();
    } else {
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        GFX_GL_CheckError();
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        GFX_GL_CheckError();
        glTexSubImage2D(
            GL_TEXTURE_2D, 0, 0, 0, desc->width, desc->height, desc->tex_format,
            desc->tex_type, data);
        GFX_GL_CheckError();
    }

    r->desc = *desc;
    if (reupload_vert) {
        M_UploadVertices(r);
    }
}

void GFX_2D_Renderer_SetPalette(
    GFX_2D_RENDERER *const r, const GFX_COLOR *const palette)
{
    ASSERT(r != nullptr);

    if (palette == nullptr) {
        if (r->use_palette) {
            GFX_GL_Program_Bind(&r->program);
            GFX_GL_Program_Uniform1i(
                &r->program, r->loc[M_UNIFORM_PALETTE_ENABLED], false);
        }
        r->use_palette = false;
        return;
    }

    if (!r->use_palette) {
        GFX_GL_Program_Bind(&r->program);
        GFX_GL_Program_Uniform1i(
            &r->program, r->loc[M_UNIFORM_PALETTE_ENABLED], true);
        GFX_GL_CheckError();
        r->use_palette = true;
    }

    glActiveTexture(GL_TEXTURE0);
    GFX_GL_Texture_Bind(&r->surface_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glActiveTexture(GL_TEXTURE1);
    GFX_GL_Texture_Bind(&r->palette_texture);
    glTexImage1D(
        GL_TEXTURE_1D, 0, GL_RGB8, 256, 0, GL_RGB, GL_UNSIGNED_BYTE, palette);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    GFX_GL_CheckError();
}

void GFX_2D_Renderer_SetRepeat(
    GFX_2D_RENDERER *const r, const int32_t x, const int32_t y)
{
    ASSERT(r != nullptr);

    if (r->repeat.x == x && r->repeat.y == y) {
        return;
    }

    r->repeat.x = x;
    r->repeat.y = y;
    M_UploadVertices(r);
}

void GFX_2D_Renderer_SetEffect(
    GFX_2D_RENDERER *const r, const GFX_2D_EFFECT effect)
{
    ASSERT(r != nullptr);

    if (r->effect != effect) {
        GFX_GL_Program_Bind(&r->program);
        GFX_GL_Program_Uniform1i(&r->program, r->loc[M_UNIFORM_EFFECT], effect);
        r->effect = effect;
    }
}

void GFX_2D_Renderer_SetTint(GFX_2D_RENDERER *const r, const GFX_COLOR color)
{
    ASSERT(r != nullptr);
    if (r->tint_color.r != color.r || r->tint_color.g != color.g
        || r->tint_color.b != color.b) {
        GFX_GL_Program_Bind(&r->program);
        GFX_GL_Program_Uniform1i(
            &r->program, r->loc[M_UNIFORM_TINT_ENABLED],
            color.r != 255 || color.g != 255 || color.b != 255);
        GFX_GL_Program_Uniform3f(
            &r->program, r->loc[M_UNIFORM_TINT_COLOR], color.r / 255.0,
            color.g / 255.0, color.b / 255.0);
        r->tint_color = color;
    }
}

void GFX_2D_Renderer_Render(GFX_2D_RENDERER *const r)
{
    ASSERT(r != nullptr);

    GFX_GL_Program_Bind(&r->program);
    GFX_GL_Buffer_Bind(&r->surface_buffer);
    GFX_GL_VertexArray_Bind(&r->vertex_format);

    glActiveTexture(GL_TEXTURE0);
    GFX_GL_Texture_Bind(&r->surface_texture);

    if (r->use_palette) {
        glActiveTexture(GL_TEXTURE1);
        GFX_GL_Texture_Bind(&r->palette_texture);
    }
    if (r->use_alpha) {
        glActiveTexture(GL_TEXTURE2);
        GFX_GL_Texture_Bind(&r->alpha_texture);
    }

    GLboolean blend = glIsEnabled(GL_BLEND);
    if (blend) {
        glDisable(GL_BLEND);
    }

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    GLboolean depth_test = glIsEnabled(GL_DEPTH_TEST);
    if (depth_test) {
        glDisable(GL_DEPTH_TEST);
    }

    glDrawArrays(GL_TRIANGLES, 0, r->vertex_count);
    GFX_GL_CheckError();

    if (blend) {
        glEnable(GL_BLEND);
    }

    if (depth_test) {
        glEnable(GL_DEPTH_TEST);
    }
    glActiveTexture(GL_TEXTURE0);
}
