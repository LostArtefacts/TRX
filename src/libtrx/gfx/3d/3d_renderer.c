#include "gfx/3d/3d_renderer.h"

#include "gfx/context.h"
#include "gfx/gl/utils.h"
#include "log.h"

#include <assert.h>
#include <stddef.h>

static void M_SelectTextureImpl(GFX_3D_RENDERER *renderer, int texture_num);

static void M_SelectTextureImpl(GFX_3D_RENDERER *renderer, int texture_num)
{
    assert(renderer);

    GFX_GL_TEXTURE *texture = NULL;
    if (texture_num == GFX_ENV_MAP_TEXTURE) {
        texture = renderer->env_map_texture;
    } else if (texture_num != GFX_NO_TEXTURE) {
        assert(texture_num >= 0);
        assert(texture_num < GFX_MAX_TEXTURES);
        texture = renderer->textures[texture_num];
    }

    if (texture == NULL) {
        glBindTexture(GL_TEXTURE_2D, 0);
        GFX_GL_CheckError();
        return;
    }

    GFX_GL_Texture_Bind(texture);
}

void GFX_3D_Renderer_Init(
    GFX_3D_RENDERER *renderer, const GFX_CONFIG *const config)
{
    LOG_INFO("");
    assert(renderer);

    renderer->config = config;

    renderer->selected_texture_num = GFX_NO_TEXTURE;
    for (int i = 0; i < GFX_MAX_TEXTURES; i++) {
        renderer->textures[i] = NULL;
    }

    GFX_GL_Sampler_Init(&renderer->sampler);
    GFX_GL_Sampler_Bind(&renderer->sampler, 0);
    GFX_GL_Sampler_Parameterf(
        &renderer->sampler, GL_TEXTURE_MAX_ANISOTROPY_EXT, 0);
    GFX_GL_Sampler_Parameteri(
        &renderer->sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    GFX_GL_Sampler_Parameteri(
        &renderer->sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    GFX_GL_Program_Init(&renderer->program);
    GFX_GL_Program_AttachShader(
        &renderer->program, GL_VERTEX_SHADER, "shaders/3d.glsl");
    GFX_GL_Program_AttachShader(
        &renderer->program, GL_FRAGMENT_SHADER, "shaders/3d.glsl");
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

void GFX_3D_Renderer_Close(GFX_3D_RENDERER *renderer)
{
    LOG_INFO("");
    assert(renderer);

    GFX_3D_VertexStream_Close(&renderer->vertex_stream);
    GFX_GL_Program_Close(&renderer->program);
    GFX_GL_Sampler_Close(&renderer->sampler);
}

void GFX_3D_Renderer_RenderBegin(GFX_3D_RENDERER *renderer)
{
    assert(renderer);
    glEnable(GL_BLEND);

    glLineWidth(renderer->config->line_width);
    glPolygonMode(
        GL_FRONT_AND_BACK,
        renderer->config->enable_wireframe ? GL_LINE : GL_FILL);
    GFX_GL_CheckError();

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

void GFX_3D_Renderer_RenderEnd(GFX_3D_RENDERER *renderer)
{
    assert(renderer);
    GFX_3D_VertexStream_RenderPending(&renderer->vertex_stream);

    GFX_GL_CheckError();
}

void GFX_3D_Renderer_ClearDepth(GFX_3D_RENDERER *renderer)
{
    GFX_3D_VertexStream_RenderPending(&renderer->vertex_stream);
    glClear(GL_DEPTH_BUFFER_BIT);
    GFX_GL_CheckError();
}

int GFX_3D_Renderer_RegisterEnvironmentMap(GFX_3D_RENDERER *const renderer)
{
    assert(renderer != NULL);
    assert(renderer->env_map_texture == NULL);

    GFX_GL_TEXTURE *texture = GFX_GL_Texture_Create(GL_TEXTURE_2D);
    renderer->env_map_texture = texture;

    GFX_3D_Renderer_RestoreTexture(renderer);
    GFX_GL_CheckError();
    return GFX_ENV_MAP_TEXTURE;
}

bool GFX_3D_Renderer_UnregisterEnvironmentMap(
    GFX_3D_RENDERER *const renderer, const int texture_num)
{
    assert(renderer != NULL);

    GFX_GL_TEXTURE *const texture = renderer->env_map_texture;
    if (texture == NULL) {
        LOG_ERROR("No environment map registered");
        return false;
    }

    if (texture_num != GFX_ENV_MAP_TEXTURE) {
        LOG_ERROR("Invalid environment map texture ID");
        return false;
    }

    // unbind texture if currently bound
    if (renderer->selected_texture_num == texture_num) {
        M_SelectTextureImpl(renderer, GFX_NO_TEXTURE);
        renderer->selected_texture_num = GFX_NO_TEXTURE;
    }

    GFX_GL_Texture_Free(texture);
    renderer->env_map_texture = NULL;
    return true;
}

void GFX_3D_Renderer_FillEnvironmentMap(GFX_3D_RENDERER *const renderer)
{
    assert(renderer != NULL);

    GFX_GL_TEXTURE *const env_map = renderer->env_map_texture;
    if (env_map != NULL) {
        GFX_3D_VertexStream_RenderPending(&renderer->vertex_stream);
        GFX_GL_Texture_LoadFromBackBuffer(env_map);
        GFX_3D_Renderer_RestoreTexture(renderer);
    }
}

int GFX_3D_Renderer_RegisterTexturePage(
    GFX_3D_RENDERER *renderer, const void *data, int width, int height)
{
    assert(renderer);
    assert(data);
    GFX_GL_TEXTURE *texture = GFX_GL_Texture_Create(GL_TEXTURE_2D);
    GFX_GL_Texture_Load(texture, data, width, height, GL_RGBA, GL_RGBA);

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

bool GFX_3D_Renderer_UnregisterTexturePage(
    GFX_3D_RENDERER *renderer, int texture_num)
{
    assert(renderer);
    assert(texture_num >= 0);
    assert(texture_num < GFX_MAX_TEXTURES);

    GFX_GL_TEXTURE *texture = renderer->textures[texture_num];
    if (!texture) {
        LOG_ERROR("Invalid texture handle");
        return false;
    }

    // unbind texture if currently bound
    if (texture_num == renderer->selected_texture_num) {
        M_SelectTextureImpl(renderer, GFX_NO_TEXTURE);
        renderer->selected_texture_num = GFX_NO_TEXTURE;
    }

    GFX_GL_Texture_Free(texture);
    renderer->textures[texture_num] = NULL;
    return true;
}

void GFX_3D_Renderer_RenderPrimStrip(
    GFX_3D_RENDERER *renderer, GFX_3D_VERTEX *vertices, int count)
{
    assert(renderer);
    assert(vertices);
    GFX_3D_VertexStream_PushPrimStrip(
        &renderer->vertex_stream, vertices, count);
}

void GFX_3D_Renderer_RenderPrimFan(
    GFX_3D_RENDERER *renderer, GFX_3D_VERTEX *vertices, int count)
{
    assert(renderer);
    assert(vertices);
    GFX_3D_VertexStream_PushPrimFan(&renderer->vertex_stream, vertices, count);
}

void GFX_3D_Renderer_RenderPrimList(
    GFX_3D_RENDERER *renderer, GFX_3D_VERTEX *vertices, int count)
{
    assert(renderer);
    assert(vertices);
    GFX_3D_VertexStream_PushPrimList(&renderer->vertex_stream, vertices, count);
}

void GFX_3D_Renderer_SelectTexture(GFX_3D_RENDERER *renderer, int texture_num)
{
    assert(renderer);
    GFX_3D_VertexStream_RenderPending(&renderer->vertex_stream);
    renderer->selected_texture_num = texture_num;
    M_SelectTextureImpl(renderer, texture_num);
}

void GFX_3D_Renderer_RestoreTexture(GFX_3D_RENDERER *renderer)
{
    assert(renderer);
    M_SelectTextureImpl(renderer, renderer->selected_texture_num);
}

void GFX_3D_Renderer_SetPrimType(
    GFX_3D_RENDERER *renderer, GFX_3D_PRIM_TYPE value)
{
    assert(renderer);
    GFX_3D_VertexStream_RenderPending(&renderer->vertex_stream);
    GFX_3D_VertexStream_SetPrimType(&renderer->vertex_stream, value);
}

void GFX_3D_Renderer_SetTextureFilter(
    GFX_3D_RENDERER *renderer, GFX_TEXTURE_FILTER filter)
{
    assert(renderer);
    GFX_3D_VertexStream_RenderPending(&renderer->vertex_stream);
    GFX_GL_Sampler_Parameteri(
        &renderer->sampler, GL_TEXTURE_MAG_FILTER,
        filter == GFX_TF_BILINEAR ? GL_LINEAR : GL_NEAREST);
    GFX_GL_Sampler_Parameteri(
        &renderer->sampler, GL_TEXTURE_MIN_FILTER,
        filter == GFX_TF_BILINEAR ? GL_LINEAR : GL_NEAREST);
    GFX_GL_Program_Uniform1i(
        &renderer->program, renderer->loc_smoothing_enabled,
        filter == GFX_TF_BILINEAR);
}

void GFX_3D_Renderer_SetDepthTestEnabled(
    GFX_3D_RENDERER *renderer, bool is_enabled)
{
    assert(renderer);
    GFX_3D_VertexStream_RenderPending(&renderer->vertex_stream);
    if (is_enabled) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
}

void GFX_3D_Renderer_SetBlendingMode(
    GFX_3D_RENDERER *const renderer, const GFX_BLEND_MODE blend_mode)
{
    assert(renderer != NULL);
    GFX_3D_VertexStream_RenderPending(&renderer->vertex_stream);

    switch (blend_mode) {
    case GFX_BLEND_MODE_OFF:
        glBlendFunc(GL_ONE, GL_ZERO);
        break;
    case GFX_BLEND_MODE_NORMAL:
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        break;
    case GFX_BLEND_MODE_MULTIPLY:
        glBlendFunc(GL_DST_COLOR, GL_SRC_COLOR);
        break;
    }
}

void GFX_3D_Renderer_SetTexturingEnabled(
    GFX_3D_RENDERER *renderer, bool is_enabled)
{
    assert(renderer);
    GFX_3D_VertexStream_RenderPending(&renderer->vertex_stream);
    GFX_GL_Program_Uniform1i(
        &renderer->program, renderer->loc_texturing_enabled, is_enabled);
}
