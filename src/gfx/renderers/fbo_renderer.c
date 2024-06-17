#include "gfx/renderers/fbo_renderer.h"

#include "config.h"
#include "gfx/common.h"
#include "gfx/context.h"
#include "gfx/gl/buffer.h"
#include "gfx/gl/gl_core_3_3.h"
#include "gfx/gl/program.h"
#include "gfx/gl/sampler.h"
#include "gfx/gl/texture.h"
#include "gfx/gl/utils.h"
#include "gfx/gl/vertex_array.h"
#include "gfx/screenshot.h"

#include <libtrx/log.h>
#include <libtrx/memory.h>

#include <SDL2/SDL_video.h>
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    GLuint fbo;
    GLuint rbo;

    GFX_GL_VertexArray vertex_array;
    GFX_GL_Buffer buffer;
    GFX_GL_Texture texture;
    GFX_GL_Sampler sampler;
    GFX_GL_Program program;
} GFX_Renderer_FBO_Context;

static void GFX_Renderer_FBO_SwapBuffers(GFX_Renderer *renderer);
static void GFX_Renderer_FBO_Init(GFX_Renderer *renderer);
static void GFX_Renderer_FBO_Shutdown(GFX_Renderer *renderer);
static void GFX_Renderer_FBO_Reset(GFX_Renderer *renderer);

static void GFX_Renderer_FBO_Render(GFX_Renderer *renderer);
static void GFX_Renderer_FBO_Bind(const GFX_Renderer *renderer);
static void GFX_Renderer_FBO_Unbind(const GFX_Renderer *renderer);

static void GFX_Renderer_FBO_SwapBuffers(GFX_Renderer *renderer)
{
    if (GFX_Context_GetScheduledScreenshotPath()) {
        GFX_Screenshot_CaptureToFile(GFX_Context_GetScheduledScreenshotPath());
        GFX_Context_ClearScheduledScreenshotPath();
    }

    GFX_Context_SwitchToWindowViewportAR();
    GFX_Renderer_FBO_Render(renderer);

    SDL_GL_SwapWindow(GFX_Context_GetWindowHandle());

    GFX_Context_SwitchToWindowViewport();
    GFX_Renderer_FBO_Unbind(renderer);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    GFX_GL_CheckError();

    GFX_Renderer_FBO_Bind(renderer);
    GFX_Context_SwitchToDisplayViewport();
}

static void GFX_Renderer_FBO_Init(GFX_Renderer *renderer)
{
    LOG_INFO("");

    assert(renderer != NULL);
    renderer->priv = (GFX_Renderer_FBO_Context *)Memory_Alloc(
        sizeof(GFX_Renderer_FBO_Context));
    GFX_Renderer_FBO_Context *priv = renderer->priv;
    assert(priv != NULL);

    int32_t fbo_width = GFX_Context_GetDisplayWidth();
    int32_t fbo_height = GFX_Context_GetDisplayHeight();

    GFX_GL_Buffer_Init(&priv->buffer, GL_ARRAY_BUFFER);
    GFX_GL_Buffer_Bind(&priv->buffer);
    GLfloat verts[] = { 0.0, 0.0, 1.0, 0.0, 0.0, 1.0,
                        0.0, 1.0, 1.0, 0.0, 1.0, 1.0 };
    GFX_GL_Buffer_Data(&priv->buffer, sizeof(verts), verts, GL_STATIC_DRAW);

    GFX_GL_VertexArray_Init(&priv->vertex_array);
    GFX_GL_VertexArray_Bind(&priv->vertex_array);
    GFX_GL_VertexArray_Attribute(
        &priv->vertex_array, 0, 2, GL_FLOAT, GL_FALSE, 0, 0);

    GFX_GL_Texture_Init(&priv->texture, GL_TEXTURE_2D);

    GFX_GL_Sampler_Init(&priv->sampler);
    GFX_GL_Sampler_Bind(&priv->sampler, 0);
    GFX_GL_Sampler_Parameteri(&priv->sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    GFX_GL_Sampler_Parameteri(&priv->sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    GFX_GL_Sampler_Parameteri(
        &priv->sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    GFX_GL_Sampler_Parameteri(
        &priv->sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    GFX_GL_Program_Init(&priv->program);
    GFX_GL_Program_AttachShader(
        &priv->program, GL_VERTEX_SHADER, "shaders/fbo.glsl");
    GFX_GL_Program_AttachShader(
        &priv->program, GL_FRAGMENT_SHADER, "shaders/fbo.glsl");
    GFX_GL_Program_Link(&priv->program);
    GFX_GL_Program_FragmentData(&priv->program, "fragColor");

    glGenFramebuffers(1, &priv->fbo);
    GFX_GL_CheckError();

    glBindFramebuffer(GL_FRAMEBUFFER, priv->fbo);
    GFX_GL_CheckError();

    GFX_GL_Texture_Load(
        &priv->texture, NULL, fbo_width, fbo_height, GL_RGB, GL_RGB);

    glFramebufferTexture2D(
        GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, priv->texture.id,
        0);
    GFX_GL_CheckError();

    glGenRenderbuffers(1, &priv->rbo);
    GFX_GL_CheckError();

    glBindRenderbuffer(GL_RENDERBUFFER, priv->rbo);
    GFX_GL_CheckError();

    glRenderbufferStorage(
        GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, fbo_width, fbo_height);
    GFX_GL_CheckError();

    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    GFX_GL_CheckError();

    glFramebufferRenderbuffer(
        GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
        priv->rbo);
    GFX_GL_CheckError();

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        LOG_ERROR("framebuffer is not complete!");
    }
}

static void GFX_Renderer_FBO_Shutdown(GFX_Renderer *renderer)
{
    LOG_INFO("");

    assert(renderer != NULL);
    GFX_Renderer_FBO_Context *priv = renderer->priv;
    assert(priv != NULL);

    if (!priv->fbo) {
        return;
    }

    glDeleteFramebuffers(1, &priv->fbo);
    priv->fbo = 0;
    GFX_GL_VertexArray_Close(&priv->vertex_array);
    GFX_GL_Buffer_Close(&priv->buffer);
    GFX_GL_Texture_Close(&priv->texture);
    GFX_GL_Sampler_Close(&priv->sampler);
    GFX_GL_Program_Close(&priv->program);

    Memory_FreePointer(&renderer->priv);
}

static void GFX_Renderer_FBO_Reset(GFX_Renderer *renderer)
{
    renderer->shutdown(renderer);
    renderer->init(renderer);
}

static void GFX_Renderer_FBO_Render(GFX_Renderer *renderer)
{
    assert(renderer != NULL);
    GFX_Renderer_FBO_Context *priv = renderer->priv;
    assert(priv != NULL);

    const GLuint filter = g_Config.rendering.fbo_filter == GFX_TF_BILINEAR
        ? GL_LINEAR
        : GL_NEAREST;

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    GFX_GL_CheckError();

    GFX_GL_Program_Bind(&priv->program);
    GFX_GL_Buffer_Bind(&priv->buffer);
    GFX_GL_VertexArray_Bind(&priv->vertex_array);
    GFX_GL_Texture_Bind(&priv->texture);
    GFX_GL_Sampler_Bind(&priv->sampler, 0);

    GFX_GL_Sampler_Parameteri(&priv->sampler, GL_TEXTURE_MAG_FILTER, filter);
    GFX_GL_Sampler_Parameteri(&priv->sampler, GL_TEXTURE_MIN_FILTER, filter);

    GLboolean blend = glIsEnabled(GL_BLEND);
    if (blend) {
        glDisable(GL_BLEND);
    }

    GLboolean depth_test = glIsEnabled(GL_DEPTH_TEST);
    if (depth_test) {
        glDisable(GL_DEPTH_TEST);
    }

    glDrawArrays(GL_TRIANGLES, 0, 6);
    GFX_GL_CheckError();

    if (blend) {
        glEnable(GL_BLEND);
    }

    if (depth_test) {
        glEnable(GL_DEPTH_TEST);
    }
    GFX_GL_CheckError();

    glBindFramebuffer(GL_FRAMEBUFFER, priv->fbo);
    GFX_GL_CheckError();
}

static void GFX_Renderer_FBO_Bind(const GFX_Renderer *renderer)
{
    assert(renderer != NULL);
    GFX_Renderer_FBO_Context *priv = renderer->priv;
    assert(priv != NULL);
    glBindFramebuffer(GL_FRAMEBUFFER, priv->fbo);
}

static void GFX_Renderer_FBO_Unbind(const GFX_Renderer *renderer)
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

GFX_Renderer g_GFX_Renderer_FBO = {
    .swap_buffers = &GFX_Renderer_FBO_SwapBuffers,
    .init = &GFX_Renderer_FBO_Init,
    .shutdown = &GFX_Renderer_FBO_Shutdown,
    .reset = &GFX_Renderer_FBO_Reset,
};
