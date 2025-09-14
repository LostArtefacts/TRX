#include "gfx/2d/2d_renderer.h"

#include "config.h"
#include "debug.h"
#include "game/output.h"
#include "gfx/context.h"
#include "gfx/gl/utils.h"
#include "log.h"
#include "memory.h"
#include "utils.h"

#include <GL/glew.h>
#include <string.h>

typedef enum {
    M_UNIFORM_BRIGHTNESS_MULTIPLIER,
    M_UNIFORM_TEXTURE_MAIN,
    M_UNIFORM_TEXTURE_SIZE,
    M_UNIFORM_EFFECT,
    M_UNIFORM_TIME,
    M_UNIFORM_TIME_INGAME,
    M_UNIFORM_VIEWPORT_SIZE,
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
    GFX_GL_PROGRAM program;

    M_VERTEX *vertices;
    int32_t vertex_count;

    bool ready;
    GFX_2D_SURFACE_DESC desc;
    struct {
        int32_t x;
        int32_t y;
    } repeat;

    // Normalized quad coordinates for rendering: x0,y0 to x1,y1 in [0,1].
    struct {
        float x0, y0, x1, y1;
    } quad;

    GFX_2D_EFFECT effect;

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

                // Apply quad scaling according to normalized coordinates
                const float px = m_Vertices[i].pos.x * x_offset + x_factor;
                const float py = m_Vertices[i].pos.y * y_offset + y_factor;
                ptr->pos.x = r->quad.x0 + (r->quad.x1 - r->quad.x0) * px;
                ptr->pos.y = r->quad.y0 + (r->quad.y1 - r->quad.y0) * py;
                ptr->uv.u = r->desc.uv[mapping[i]].u;
                ptr->uv.v = r->desc.uv[mapping[i]].v;

                ptr++;
            }
        }
    }
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
    r->repeat.x = 1;
    r->repeat.y = 1;
    r->quad.x0 = 0.0f;
    r->quad.y0 = 0.0f;
    r->quad.x1 = 1.0f;
    r->quad.y1 = 1.0f;

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

    GFX_GL_Program_Init(&r->program);
    GFX_GL_Program_AttachShader(
        &r->program, GL_VERTEX_SHADER, "shaders/2d.glsl");
    GFX_GL_Program_AttachShader(
        &r->program, GL_FRAGMENT_SHADER, "shaders/2d.glsl");
    GFX_GL_Program_FragmentData(&r->program, "outColor");
    GFX_GL_Program_Link(&r->program);

    struct {
        M_UNIFORM loc;
        const char *name;
    } uniforms[] = {
        { M_UNIFORM_BRIGHTNESS_MULTIPLIER, "uBrightnessMultiplier" },
        { M_UNIFORM_TEXTURE_MAIN, "texMain" },
        { M_UNIFORM_TEXTURE_SIZE, "uTexSize" },
        { M_UNIFORM_EFFECT, "uEffect" },
        { M_UNIFORM_TIME, "uTime" },
        { M_UNIFORM_TIME_INGAME, "uTimeInGame" },
        { M_UNIFORM_VIEWPORT_SIZE, "uViewportSize" },
        { -1, nullptr },
    };
    for (int32_t i = 0; uniforms[i].name != nullptr; i++) {
        r->loc[uniforms[i].loc] =
            GFX_GL_Program_UniformLocation(&r->program, uniforms[i].name);
        GFX_GL_CheckError();
    }

    GFX_GL_Program_Bind(&r->program);
    GFX_GL_Program_Uniform1i(&r->program, r->loc[M_UNIFORM_TEXTURE_MAIN], 0);
    GFX_GL_Program_Uniform4f(
        &r->program, r->loc[M_UNIFORM_TEXTURE_SIZE], 0.0f, 0.0f, 1.0f, 1.0f);
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
    GFX_GL_Program_Close(&r->program);
    Memory_FreePointer(&r->vertices);
    Memory_Free(r);
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
    if (!r->ready) {
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

    r->ready = true;
    r->desc = *desc;
    if (reupload_vert) {
        M_UploadVertices(r);
    }
}

void GFX_2D_Renderer_SetTextureSize(
    GFX_2D_RENDERER *const r, const GFX_TEXTURE_SIZE *const size)
{
    ASSERT(r != nullptr);
    GFX_GL_Program_Bind(&r->program);
    if (size == nullptr) {
        GFX_GL_Program_Uniform4f(
            &r->program, r->loc[M_UNIFORM_TEXTURE_SIZE], 0.0f, 0.0f, 1.0f,
            1.0f);
    } else {
        GFX_GL_Program_Uniform4f(
            &r->program, r->loc[M_UNIFORM_TEXTURE_SIZE], size->x0, size->y0,
            size->x1, size->y1);
    }
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

void GFX_2D_Renderer_SetQuad(
    GFX_2D_RENDERER *const r, const float x0, const float y0, const float x1,
    const float y1)
{
    ASSERT(r != nullptr);
    if (r->quad.x0 == x0 && r->quad.y0 == y0 && r->quad.x1 == x1
        && r->quad.y1 == y1) {
        return;
    }
    r->quad.x0 = x0;
    r->quad.y0 = y0;
    r->quad.x1 = x1;
    r->quad.y1 = y1;
    M_UploadVertices(r);
}

void GFX_2D_Renderer_SetEffect(GFX_2D_RENDERER *const r, const uint32_t effect)
{
    ASSERT(r != nullptr);

    if (r->effect != effect) {
        GFX_GL_Program_Bind(&r->program);
        GFX_GL_Program_Uniform1i(&r->program, r->loc[M_UNIFORM_EFFECT], effect);
        r->effect = effect;
    }
}

void GFX_2D_Renderer_Render(GFX_2D_RENDERER *const r)
{
    ASSERT(r != nullptr);

    GFX_GL_Program_Bind(&r->program);
    GFX_GL_Program_Uniform1f(
        &r->program, r->loc[M_UNIFORM_TIME], Output_GetTime());
    GFX_GL_Program_Uniform1f(
        &r->program, r->loc[M_UNIFORM_TIME_INGAME], Output_GetTimeInGame());
    GFX_GL_Program_Uniform2f(
        &r->program, r->loc[M_UNIFORM_VIEWPORT_SIZE],
        Viewport_GetWidth(VIEWPORT_GAME), Viewport_GetHeight(VIEWPORT_GAME));

    GFX_GL_Program_Uniform1i(&r->program, r->loc[M_UNIFORM_EFFECT], r->effect);
    GFX_GL_Buffer_Bind(&r->surface_buffer);
    GFX_GL_VertexArray_Bind(&r->vertex_format);

    glActiveTexture(GL_TEXTURE0);
    GFX_GL_Texture_Bind(&r->surface_texture);
    GFX_GL_Program_Uniform1f(
        &r->program, r->loc[M_UNIFORM_BRIGHTNESS_MULTIPLIER],
        g_Config.visuals.brightness);

    GLboolean blend = glIsEnabled(GL_BLEND);
    if (blend) {
        glDisable(GL_BLEND);
    }

    GLint bound_polygon_mode[2];
    glGetIntegerv(GL_POLYGON_MODE, &bound_polygon_mode[0]);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    GLboolean depth_test = glIsEnabled(GL_DEPTH_TEST);
    if (depth_test) {
        glDisable(GL_DEPTH_TEST);
    }

    glDrawArrays(GL_TRIANGLES, 0, r->vertex_count);
    glPolygonMode(GL_FRONT_AND_BACK, bound_polygon_mode[0]);
    GFX_GL_CheckError();
}
