#include "gfx/3d/3d_renderer.h"

#include "config.h"
#include "gfx/context.h"
#include "gfx/gl/utils.h"

#include <assert.h>

static void GFX_3D_Renderer_SelectTextureImpl(
    GFX_3D_Renderer *renderer, int texture_num);

static void GFX_3D_Renderer_SelectTextureImpl(
    GFX_3D_Renderer *renderer, int texture_num)
{
    if (texture_num == GFX_NO_TEXTURE) {
        glBindTexture(GL_TEXTURE_2D, 0);
        return;
    }

    assert(texture_num >= 0);
    assert(texture_num < GFX_MAX_TEXTURES);

    GFX_GL_Texture *texture = renderer->textures[texture_num];
    GFX_GL_Texture_Bind(texture);
}

void GFX_3D_Renderer_Init(GFX_3D_Renderer *renderer)
{
    // TODO: make me configurable
    renderer->wireframe = false;
    renderer->selected_texture_num = GFX_NO_TEXTURE;
    for (int i = 0; i < GFX_MAX_TEXTURES; i++) {
        renderer->textures[i] = NULL;
    }

    GFX_GL_Sampler_Init(&renderer->sampler);
    GFX_GL_Sampler_Bind(&renderer->sampler, 0);
    GFX_GL_Sampler_Parameterf(
        &renderer->sampler, GL_TEXTURE_MAX_ANISOTROPY_EXT,
        g_Config.rendering.anisotropy_filter);

    GFX_GL_Program_Init(&renderer->program);
    GFX_GL_Program_AttachShader(
        &renderer->program, GL_VERTEX_SHADER, "shaders\\3d.vsh");
    GFX_GL_Program_AttachShader(
        &renderer->program, GL_FRAGMENT_SHADER, "shaders\\3d.fsh");
    GFX_GL_Program_Link(&renderer->program);

    renderer->loc_mat_projection =
        GFX_GL_Program_UniformLocation(&renderer->program, "matProjection");
    renderer->loc_mat_model_view =
        GFX_GL_Program_UniformLocation(&renderer->program, "matModelView");
    renderer->loc_texturing_enabled =
        GFX_GL_Program_UniformLocation(&renderer->program, "texturingEnabled");
    renderer->loc_smoothing_enabled =
        GFX_GL_Program_UniformLocation(&renderer->program, "smoothingEnabled");

    GFX_GL_Program_FragmentData(&renderer->program, "fragColor");
    GFX_GL_Program_Bind(&renderer->program);

    // negate Z axis so the model is rendered behind the viewport, which is
    // better than having a negative z_near in the ortho matrix, which seems
    // to mess up depth testing
    GLfloat model_view[4][4] = {
        { +1.0f, +0.0f, +0.0f, +0.0f },
        { +0.0f, +1.0f, +0.0f, +0.0f },
        { +0.0f, +0.0f, -1.0f, +0.0f },
        { +0.0f, +0.0f, +0.0f, +1.0f },
    };
    GFX_GL_Program_UniformMatrix4fv(
        &renderer->program, renderer->loc_mat_model_view, 1, GL_FALSE,
        &model_view[0][0]);

    GFX_3D_VertexStream_Init(&renderer->vertex_stream);

    GFX_GL_CheckError();
}

void GFX_3D_Renderer_Close(GFX_3D_Renderer *renderer)
{
    GFX_3D_VertexStream_Close(&renderer->vertex_stream);
    GFX_GL_Program_Close(&renderer->program);
    GFX_GL_Sampler_Close(&renderer->sampler);
}

void GFX_3D_Renderer_RenderBegin(GFX_3D_Renderer *renderer)
{
    glEnable(GL_BLEND);

    if (renderer->wireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    GFX_GL_Program_Bind(&renderer->program);
    GFX_3D_VertexStream_Bind(&renderer->vertex_stream);
    GFX_GL_Sampler_Bind(&renderer->sampler, 0);

    GFX_3D_Renderer_RestoreTexture(renderer);

    const float left = 0.0f;
    const float top = 0.0f;
    const float right = GFX_Context_GetDisplayWidth();
    const float bottom = GFX_Context_GetDisplayHeight();
    const float z_near = -1e6;
    const float z_far = 1e6;
    GLfloat projection[4][4] = {
        { 2.0f / (right - left), 0.0f, 0.0f, 0.0f },
        { 0.0f, 2.0f / (top - bottom), 0.0f, 0.0f },
        { 0.0f, 0.0f, -2.0f / (z_far - z_near), 0.0f },
        { -(right + left) / (right - left), -(top + bottom) / (top - bottom),
          -(z_far + z_near) / (z_far - z_near), 1.0f }
    };

    GFX_GL_Program_UniformMatrix4fv(
        &renderer->program, renderer->loc_mat_projection, 1, GL_FALSE,
        &projection[0][0]);

    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);

    GFX_GL_CheckError();
}

void GFX_3D_Renderer_RenderEnd(GFX_3D_Renderer *renderer)
{
    GFX_3D_VertexStream_RenderPending(&renderer->vertex_stream);

    if (renderer->wireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    GFX_GL_CheckError();
}

int GFX_3D_Renderer_TextureReg(
    GFX_3D_Renderer *renderer, const void *data, int width, int height)
{
    GFX_GL_Texture *texture = GFX_GL_Texture_Create(GL_TEXTURE_2D);
    GFX_GL_Texture_Bind(texture);
    GFX_GL_Texture_Load(texture, data, width, height);

    int texture_num = GFX_NO_TEXTURE;
    for (int i = 0; i < GFX_MAX_TEXTURES; i++) {
        if (!renderer->textures[i]) {
            renderer->textures[i] = texture;
            texture_num = i;
            break;
        }
    }

    GFX_3D_Renderer_RestoreTexture(renderer);

    GFX_GL_CheckError();
    return texture_num;
}

bool GFX_3D_Renderer_TextureUnreg(GFX_3D_Renderer *renderer, int texture_num)
{
    assert(texture_num >= 0);
    assert(texture_num < GFX_MAX_TEXTURES);

    GFX_GL_Texture *texture = renderer->textures[texture_num];
    if (!texture) {
        LOG_ERROR("Invalid texture handle");
        return false;
    }

    // unbind texture if currently bound
    if (texture_num == renderer->selected_texture_num) {
        GFX_3D_Renderer_SelectTextureImpl(renderer, GFX_NO_TEXTURE);
        renderer->selected_texture_num = GFX_NO_TEXTURE;
    }

    GFX_GL_Texture_Free(texture);
    renderer->textures[texture_num] = NULL;
    return true;
}

void GFX_3D_Renderer_RenderPrimStrip(
    GFX_3D_Renderer *renderer, GFX_3D_Vertex *vertices, int count)
{
    GFX_Context_SetRendered();
    GFX_3D_VertexStream_PushPrimStrip(
        &renderer->vertex_stream, vertices, count);
}

void GFX_3D_Renderer_RenderPrimList(
    GFX_3D_Renderer *renderer, GFX_3D_Vertex *vertices, int count)
{
    GFX_Context_SetRendered();
    GFX_3D_VertexStream_PushPrimList(&renderer->vertex_stream, vertices, count);
}

void GFX_3D_Renderer_SelectTexture(GFX_3D_Renderer *renderer, int texture_num)
{
    GFX_3D_VertexStream_RenderPending(&renderer->vertex_stream);
    renderer->selected_texture_num = texture_num;
    GFX_3D_Renderer_SelectTextureImpl(renderer, texture_num);
}

void GFX_3D_Renderer_RestoreTexture(GFX_3D_Renderer *renderer)
{
    GFX_3D_Renderer_SelectTextureImpl(renderer, renderer->selected_texture_num);
}

void GFX_3D_Renderer_SetPrimType(
    GFX_3D_Renderer *renderer, GFX_3D_PrimType value)
{
    GFX_3D_VertexStream_RenderPending(&renderer->vertex_stream);
    GFX_3D_VertexStream_SetPrimType(&renderer->vertex_stream, value);
}

void GFX_3D_Renderer_SetSmoothingEnabled(
    GFX_3D_Renderer *renderer, bool is_enabled)
{
    GFX_3D_VertexStream_RenderPending(&renderer->vertex_stream);
    GFX_GL_Sampler_Parameteri(
        &renderer->sampler, GL_TEXTURE_MAG_FILTER,
        is_enabled ? GL_LINEAR : GL_NEAREST);
    GFX_GL_Sampler_Parameteri(
        &renderer->sampler, GL_TEXTURE_MIN_FILTER,
        is_enabled ? GL_LINEAR : GL_NEAREST);
    GFX_GL_Program_Uniform1i(
        &renderer->program, renderer->loc_smoothing_enabled, is_enabled);
}

void GFX_3D_Renderer_SetBlendingEnabled(
    GFX_3D_Renderer *renderer, bool is_enabled)
{
    GFX_3D_VertexStream_RenderPending(&renderer->vertex_stream);
    if (is_enabled) {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    } else {
        glBlendFunc(GL_ONE, GL_ZERO);
    }
}

void GFX_3D_Renderer_SetTexturingEnabled(
    GFX_3D_Renderer *renderer, bool is_enabled)
{
    GFX_3D_VertexStream_RenderPending(&renderer->vertex_stream);
    GFX_GL_Program_Uniform1i(
        &renderer->program, renderer->loc_texturing_enabled, is_enabled);
}

void GFX_3D_Renderer_RenderEmpty()
{
    GFX_Context_SetRendered();
}