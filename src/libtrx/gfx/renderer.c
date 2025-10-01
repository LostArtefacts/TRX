#include "gfx/renderer.h"

#include "debug.h"
#include "gfx/common.h"
#include "gfx/context.h"
#include "gfx/gl/buffer.h"
#include "gfx/gl/fbo.h"
#include "gfx/gl/program.h"
#include "gfx/gl/sampler.h"
#include "gfx/gl/texture.h"
#include "gfx/gl/utils.h"
#include "gfx/gl/vertex_array.h"
#include "gfx/screenshot.h"
#include "log.h"
#include "memory.h"

#include <GL/glew.h>
#include <SDL2/SDL_video.h>
#include <stdint.h>

typedef struct {
    const GFX_CONFIG *config;

    GFX_GL_FBO geometry_fbo;
    GFX_GL_FBO ui_fbo;

    // Full-screen quad resources for blitting FBOs to default framebuffer.
    GFX_GL_VERTEX_ARRAY vertex_array;
    GFX_GL_BUFFER buffer;
    GFX_GL_SAMPLER sampler;
    GFX_GL_PROGRAM program;
} M_CONTEXT;

static void M_Blit(const M_CONTEXT *const p, const GFX_GL_FBO *const fbo)
{
    GFX_GL_Texture_Bind(&fbo->texture);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    GFX_GL_CheckError();
}

static void M_UpdateFBOSizes(GFX_RENDERER *renderer)
{
    M_CONTEXT *const p = renderer->priv;
    const GFX_CONFIG *const config = p->config;

    VIEWPORT_RECT rect;
    rect = Viewport_GetRect(VIEWPORT_GAME);
    GFX_GL_FBO_ResizeIfNeeded(&p->geometry_fbo, rect.width, rect.height);
    rect = Viewport_GetRect(VIEWPORT_UI);
    GFX_GL_FBO_ResizeIfNeeded(&p->ui_fbo, rect.width, rect.height);
}

static void M_Render(GFX_RENDERER *renderer)
{
    ASSERT(renderer != nullptr);
    M_CONTEXT *const p = renderer->priv;
    ASSERT(p != nullptr);

    const GLuint filter =
        p->config->display_filter == GFX_TF_BILINEAR ? GL_LINEAR : GL_NEAREST;

    GFX_GL_FBO_Unbind();

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    GFX_GL_CheckError();

    GFX_GL_Program_Bind(&p->program);
    GFX_GL_Buffer_Bind(&p->buffer);
    GFX_GL_VertexArray_Bind(&p->vertex_array);
    glActiveTexture(GL_TEXTURE0);
    glDisable(GL_DEPTH_TEST);

    GFX_GL_Sampler_Bind(&p->sampler, 0);
    GFX_GL_Sampler_Parameteri(&p->sampler, GL_TEXTURE_MAG_FILTER, filter);
    GFX_GL_Sampler_Parameteri(&p->sampler, GL_TEXTURE_MIN_FILTER, filter);

    VIEWPORT_RECT rect = Viewport_GetRect(VIEWPORT_TARGET);
    glViewport(rect.x, rect.y, rect.width, rect.height);
    GFX_GL_CheckError();

    // Composite geometry FBO (opaque)
    glDisable(GL_BLEND);
    M_Blit(p, &p->geometry_fbo);

    // Composite UI FBO (with premultiplied alpha blending)
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    M_Blit(p, &p->ui_fbo);
    glDisable(GL_BLEND);

    if (GFX_Context_GetScheduledScreenshotPath() != nullptr) {
        GFX_Context_SwitchToViewport(VIEWPORT_TARGET);
        GFX_Screenshot_CaptureToFile(GFX_Context_GetScheduledScreenshotPath());
        GFX_Context_ClearScheduledScreenshotPath();
    }
}

static void M_SwapBuffers(GFX_RENDERER *const renderer)
{
    M_CONTEXT *const p = renderer->priv;

    M_Render(renderer);
    SDL_GL_SwapWindow(GFX_Context_GetWindowHandle());
    M_UpdateFBOSizes(renderer);

    GFX_Context_SwitchToViewport(VIEWPORT_WINDOW);
    GFX_Context_Clear();

    // Rebind geometry FBO for the next frame
    GFX_Renderer_BindGeometryFbo();
    GFX_Context_SwitchToViewport(VIEWPORT_GAME);
    GFX_Context_Clear();
}

static void M_Init(GFX_RENDERER *const renderer, const GFX_CONFIG *const config)
{
    ASSERT(renderer != nullptr);
    renderer->priv = (M_CONTEXT *)Memory_Alloc(sizeof(M_CONTEXT));
    M_CONTEXT *const p = renderer->priv;
    ASSERT(p != nullptr);

    p->config = config;

    GFX_GL_Buffer_Init(&p->buffer, GL_ARRAY_BUFFER);
    GFX_GL_Buffer_Bind(&p->buffer);
    const GLfloat verts[] = {
        0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
    };
    GFX_GL_Buffer_Data(&p->buffer, sizeof(verts), verts, GL_STATIC_DRAW);

    GFX_GL_VertexArray_Init(&p->vertex_array);
    GFX_GL_VertexArray_Bind(&p->vertex_array);
    GFX_GL_VertexArray_Attribute(
        &p->vertex_array, 0, 2, GL_FLOAT, GL_FALSE, 0, 0);

    GFX_GL_Sampler_Init(&p->sampler);
    GFX_GL_Sampler_Bind(&p->sampler, 0);
    GFX_GL_Sampler_Parameteri(&p->sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    GFX_GL_Sampler_Parameteri(&p->sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    GFX_GL_Sampler_Parameteri(&p->sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    GFX_GL_Sampler_Parameteri(&p->sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    GFX_GL_Program_Init(&p->program);
    GFX_GL_Program_AttachShader(
        &p->program, GL_VERTEX_SHADER, "shaders/fbo.glsl");
    GFX_GL_Program_AttachShader(
        &p->program, GL_FRAGMENT_SHADER, "shaders/fbo.glsl");
    GFX_GL_Program_FragmentData(&p->program, "outColor");
    GFX_GL_Program_Link(&p->program);
    GFX_GL_Program_Bind(&p->program);
    GFX_GL_Program_Uniform1i(
        &p->program, GFX_GL_Program_UniformLocation(&p->program, "tex0"), 0);

    VIEWPORT_RECT rect;
    rect = Viewport_GetRect(VIEWPORT_GAME);
    GFX_GL_FBO_Init(
        &p->geometry_fbo, rect.width, rect.height, GL_RGBA8, GL_RGBA, true);

    rect = Viewport_GetRect(VIEWPORT_UI);
    GFX_GL_FBO_Init(
        &p->ui_fbo, rect.width, rect.height, GL_RGBA8, GL_RGBA, false);
}

static void M_Shutdown(GFX_RENDERER *renderer)
{
    LOG_INFO("");

    ASSERT(renderer != nullptr);
    M_CONTEXT *const p = renderer->priv;
    ASSERT(p != nullptr);

    GFX_GL_FBO_Close(&p->geometry_fbo);
    GFX_GL_FBO_Close(&p->ui_fbo);
    GFX_GL_VertexArray_Close(&p->vertex_array);

    Memory_FreePointer(&renderer->priv);
}

GFX_RENDERER g_GFX_Renderer = {
    .swap_buffers = &M_SwapBuffers,
    .init = &M_Init,
    .shutdown = &M_Shutdown,
};

void GFX_Renderer_BindGeometryFbo(void)
{
    M_CONTEXT *const p = (M_CONTEXT *)g_GFX_Renderer.priv;
    GFX_GL_FBO_Bind(&p->geometry_fbo);
}

void GFX_Renderer_BindUiFbo(void)
{
    M_CONTEXT *const p = (M_CONTEXT *)g_GFX_Renderer.priv;
    GFX_GL_FBO_Bind(&p->ui_fbo);
}
