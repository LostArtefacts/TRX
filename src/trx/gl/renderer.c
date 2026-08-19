#include <trx/gl/renderer.h>

#include <trx/core/log.h>
#include <trx/core/memory.h>
#include <trx/debug.h>
#include <trx/gl/buffer.h>
#include <trx/gl/context.h>
#include <trx/gl/enum.h>
#include <trx/gl/fbo.h>
#include <trx/gl/program.h>
#include <trx/gl/sampler.h>
#include <trx/gl/screenshot.h>
#include <trx/gl/texture.h>
#include <trx/gl/utils.h>
#include <trx/gl/vertex_array.h>

#include <GL/glew.h>
#include <SDL2/SDL_video.h>
#include <stdint.h>

typedef struct {
    const TRX_GL_CONFIG *config;

    TRX_GL_FBO geometry_fbo;
    TRX_GL_FBO ui_fbo;
    // Holds the geometry framebuffer brought down to the scene resolution.
    // Only allocated while there is something to resolve.
    TRX_GL_FBO resolve_fbo;
    // The size the geometry framebuffer resolves to, recorded when it was
    // sized. The viewport moves ahead of the framebuffer, which is only
    // rebuilt once the frame it belongs to has been presented.
    int32_t scene_width;
    int32_t scene_height;
    // Whether resolve_fbo already holds this frame's scene.
    bool scene_resolved;

    // Full-screen quad resources for blitting FBOs to default framebuffer.
    TRX_GL_VERTEX_ARRAY vertex_array;
    TRX_GL_BUFFER buffer;
    TRX_GL_SAMPLER sampler;
    TRX_GL_PROGRAM program;
    GLint loc_dither_mode;
    GLint loc_supersample;
    GLuint composite_fbo;
} M_CONTEXT;

static void M_Blit(const M_CONTEXT *const p, const TRX_GL_FBO *const fbo)
{
    TRX_GL_Texture_Bind(&fbo->texture);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    TRX_GL_CheckError();
}

static void M_BindQuadState(M_CONTEXT *const p)
{
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    TRX_GL_Program_Bind(&p->program);
    TRX_GL_Buffer_Bind(&p->buffer);
    TRX_GL_VertexArray_Bind(&p->vertex_array);
    TRX_GL_Sampler_Bind(&p->sampler, 0);
    glActiveTexture(GL_TEXTURE0);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    TRX_GL_CheckError();
}

// Multisampling and supersampling are alternatives, so a scene rasterized at
// more than the scene resolution is rasterized with one sample per pixel.
static int32_t M_GetGeometrySamples(const M_CONTEXT *const p)
{
    const VIEWPORT_RECT game = Viewport_GetRect(VIEWPORT_GAME);
    const VIEWPORT_RECT scene = Viewport_GetRect(VIEWPORT_SCENE);
    return game.width > scene.width ? 1 : p->config->multisampling_factor;
}

static void M_SizeGeometryFbo(M_CONTEXT *const p)
{
    const VIEWPORT_RECT game = Viewport_GetRect(VIEWPORT_GAME);
    const VIEWPORT_RECT scene = Viewport_GetRect(VIEWPORT_SCENE);
    TRX_GL_FBO_ResizeIfNeeded(
        &p->geometry_fbo, game.width, game.height, M_GetGeometrySamples(p));
    p->scene_width = scene.width;
    p->scene_height = scene.height;
}

// Bring the geometry framebuffer down to the scene resolution the composite
// pass expects: averaging the samples of a multisampled buffer, or box-
// filtering a supersampled one down to the pixel grid the player is meant to
// see. Returns the geometry framebuffer itself when neither is in play,
// leaving that path exactly as it was.
static const TRX_GL_FBO *M_ResolveScene(M_CONTEXT *const p)
{
    const int32_t width = p->scene_width;
    const int32_t height = p->scene_height;
    const int32_t factor = width > 0 ? p->geometry_fbo.width / width : 1;
    if (factor <= 1 && p->geometry_fbo.samples <= 1) {
        if (p->resolve_fbo.fbo != 0) {
            TRX_GL_FBO_Close(&p->resolve_fbo);
        }
        return &p->geometry_fbo;
    }

    if (p->scene_resolved) {
        return &p->resolve_fbo;
    }

    if (p->resolve_fbo.fbo == 0) {
        TRX_GL_FBO_Init(
            &p->resolve_fbo, width, height, 1, GL_RGBA8, GL_RGBA, false);
    } else {
        TRX_GL_FBO_ResizeIfNeeded(&p->resolve_fbo, width, height, 1);
    }

    if (p->geometry_fbo.samples > 1) {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, p->geometry_fbo.fbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, p->resolve_fbo.fbo);
        glBlitFramebuffer(
            0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT,
            GL_NEAREST);
        TRX_GL_CheckError();
    } else {
        M_BindQuadState(p);
        TRX_GL_FBO_Bind(&p->resolve_fbo);
        glViewport(0, 0, width, height);
        TRX_GL_Program_Uniform1i(&p->program, p->loc_supersample, factor);
        TRX_GL_Program_Uniform1i(
            &p->program, p->loc_dither_mode, DITHER_MODE_DISABLED);
        M_Blit(p, &p->geometry_fbo);
        TRX_GL_Program_Uniform1i(&p->program, p->loc_supersample, 1);
    }

    p->scene_resolved = true;
    return &p->resolve_fbo;
}

static void M_UpdateFBOSizes(TRX_GL_RENDERER *renderer)
{
    M_CONTEXT *const p = renderer->priv;

    M_SizeGeometryFbo(p);
    const VIEWPORT_RECT rect = Viewport_GetRect(VIEWPORT_UI);
    TRX_GL_FBO_ResizeIfNeeded(&p->ui_fbo, rect.width, rect.height, 1);
}

// Draws the scene and the UI over it into the given framebuffer. Resolving the
// scene binds framebuffers of its own, so the destination is bound here rather
// than by the caller.
static void M_Composite(
    M_CONTEXT *const p, const GLuint dst_fbo, const VIEWPORT_RECT rect)
{
    const GLuint filter = p->config->display_filter == TEXTURE_FILTER_BILINEAR
        ? GL_LINEAR
        : GL_NEAREST;

    const TRX_GL_FBO *const scene_fbo = M_ResolveScene(p);

    M_BindQuadState(p);
    glBindFramebuffer(GL_FRAMEBUFFER, dst_fbo);

    TRX_GL_Sampler_Parameteri(&p->sampler, GL_TEXTURE_MAG_FILTER, filter);
    TRX_GL_Sampler_Parameteri(&p->sampler, GL_TEXTURE_MIN_FILTER, filter);

    TRX_GL_Program_Uniform1i(
        &p->program, p->loc_dither_mode, p->config->dither_mode);

    glViewport(rect.x, rect.y, rect.width, rect.height);
    TRX_GL_CheckError();

    // Composite scene FBO (opaque)
    M_Blit(p, scene_fbo);

    // Composite UI FBO (with premultiplied alpha blending)
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    M_Blit(p, &p->ui_fbo);
    glDisable(GL_BLEND);
}

static void M_Render(TRX_GL_RENDERER *renderer)
{
    ASSERT(renderer != nullptr);
    M_CONTEXT *const p = renderer->priv;
    ASSERT(p != nullptr);

    M_Composite(p, 0, Viewport_GetRect(VIEWPORT_TARGET));

    if (TRX_GL_Context_GetScheduledScreenshotPath() != nullptr) {
        TRX_GL_Context_SwitchToViewport(VIEWPORT_TARGET);
        SHOULD(TRX_GL_Screenshot_CaptureToFile(
            TRX_GL_Context_GetScheduledScreenshotPath()));
        TRX_GL_Context_ClearScheduledScreenshotPath();
    }
}

// The framebuffers keep the frame that was just presented until the next one
// draws over them, so that frame can be composited a second time for a
// snapshot. Output_BeginScene is what prepares them for the next frame.
static void M_SwapBuffers(TRX_GL_RENDERER *const renderer)
{
    M_CONTEXT *const p = renderer->priv;

    M_Render(renderer);
    SDL_GL_SwapWindow(TRX_GL_Context_GetWindowHandle());
    M_UpdateFBOSizes(renderer);

    TRX_GL_Context_SwitchToViewport(VIEWPORT_WINDOW);
    TRX_GL_Context_Clear();

    TRX_GL_FBO_Bind(&p->geometry_fbo);
    TRX_GL_Context_SwitchToViewport(VIEWPORT_GAME);
}

static RESULT M_Init(
    TRX_GL_RENDERER *const renderer, const TRX_GL_CONFIG *const config)
{
    ASSERT(renderer != nullptr);
    renderer->priv = (M_CONTEXT *)Memory_Alloc(sizeof(M_CONTEXT));
    M_CONTEXT *const p = renderer->priv;
    ASSERT(p != nullptr);

    p->config = config;

    glEnable(GL_MULTISAMPLE);
    TRX_GL_CheckError();

    TRX_GL_Buffer_Init(&p->buffer, GL_ARRAY_BUFFER);
    TRX_GL_Buffer_Bind(&p->buffer);
    const GLfloat verts[] = {
        0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
    };
    TRX_GL_Buffer_Data(&p->buffer, sizeof(verts), verts, GL_STATIC_DRAW);

    TRX_GL_VertexArray_Init(&p->vertex_array);
    TRX_GL_VertexArray_Bind(&p->vertex_array);
    TRX_GL_VertexArray_Attribute(
        &p->vertex_array, 0, 2, GL_FLOAT, GL_FALSE, 0, 0);

    TRX_GL_Sampler_Init(&p->sampler);
    TRX_GL_Sampler_Bind(&p->sampler, 0);
    TRX_GL_Sampler_Parameteri(&p->sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    TRX_GL_Sampler_Parameteri(&p->sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    TRX_GL_Sampler_Parameteri(&p->sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    TRX_GL_Sampler_Parameteri(&p->sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    MUST(TRX_GL_Program_Init(&p->program));
    MUST(TRX_GL_Program_AttachShader(
        &p->program, GL_VERTEX_SHADER, "fbo.glsl", nullptr));
    MUST(TRX_GL_Program_AttachShader(
        &p->program, GL_FRAGMENT_SHADER, "fbo.glsl", nullptr));
    TRX_GL_Program_FragmentData(&p->program, "outColor");
    MUST(TRX_GL_Program_Link(&p->program));
    TRX_GL_Program_Bind(&p->program);
    TRX_GL_Program_Uniform1i(
        &p->program, TRX_GL_Program_UniformLocation(&p->program, "uTex0"), 0);
    p->loc_dither_mode =
        TRX_GL_Program_UniformLocation(&p->program, "uDitherMode");
    p->loc_supersample =
        TRX_GL_Program_UniformLocation(&p->program, "uSupersample");
    TRX_GL_Program_Uniform1i(&p->program, p->loc_supersample, 1);

    VIEWPORT_RECT rect;
    rect = Viewport_GetRect(VIEWPORT_GAME);
    TRX_GL_FBO_Init(
        &p->geometry_fbo, rect.width, rect.height, M_GetGeometrySamples(p),
        GL_RGBA8, GL_RGBA, true);
    p->scene_width = Viewport_GetWidth(VIEWPORT_SCENE);
    p->scene_height = Viewport_GetHeight(VIEWPORT_SCENE);

    rect = Viewport_GetRect(VIEWPORT_UI);
    TRX_GL_FBO_Init(
        &p->ui_fbo, rect.width, rect.height, 1, GL_RGBA8, GL_RGBA, false);
    return OK;
}

static void M_Shutdown(TRX_GL_RENDERER *renderer)
{
    LOG_INFO("");

    ASSERT(renderer != nullptr);
    M_CONTEXT *const p = renderer->priv;
    ASSERT(p != nullptr);

    TRX_GL_FBO_Close(&p->geometry_fbo);
    TRX_GL_FBO_Close(&p->ui_fbo);
    TRX_GL_FBO_Close(&p->resolve_fbo);
    if (p->composite_fbo != 0) {
        glDeleteFramebuffers(1, &p->composite_fbo);
        p->composite_fbo = 0;
    }
    TRX_GL_Program_Close(&p->program);
    TRX_GL_Sampler_Close(&p->sampler);
    TRX_GL_Buffer_Close(&p->buffer);
    TRX_GL_VertexArray_Close(&p->vertex_array);

    Memory_FreePointer(&renderer->priv);
}

TRX_GL_RENDERER g_TRX_GL_Renderer = {
    .swap_buffers = &M_SwapBuffers,
    .init = &M_Init,
    .shutdown = &M_Shutdown,
};

void TRX_GL_Renderer_BindGeometryFbo(void)
{
    M_CONTEXT *const p = (M_CONTEXT *)g_TRX_GL_Renderer.priv;
    // Anything about to draw the scene again invalidates what was resolved
    // from it.
    p->scene_resolved = false;
    TRX_GL_FBO_Bind(&p->geometry_fbo);
}

void TRX_GL_Renderer_BindUiFbo(void)
{
    M_CONTEXT *const p = (M_CONTEXT *)g_TRX_GL_Renderer.priv;
    TRX_GL_FBO_Bind(&p->ui_fbo);
}

void TRX_GL_Renderer_SyncFboSizes(void)
{
    M_UpdateFBOSizes(&g_TRX_GL_Renderer);
}

void TRX_GL_Renderer_CompositeToTexture(
    const TRX_GL_TEXTURE *const texture, const int32_t width,
    const int32_t height)
{
    M_CONTEXT *const p = (M_CONTEXT *)g_TRX_GL_Renderer.priv;
    if (texture == nullptr || !texture->initialized || width <= 0
        || height <= 0) {
        return;
    }

    if (p->composite_fbo == 0) {
        glGenFramebuffers(1, &p->composite_fbo);
    }

    GLint prev_fbo = 0;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &prev_fbo);

    glBindFramebuffer(GL_FRAMEBUFFER, p->composite_fbo);
    glFramebufferTexture2D(
        GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture->id, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE) {
        M_Composite(
            p, p->composite_fbo,
            (VIEWPORT_RECT) { .width = width, .height = height });
    } else {
        LOG_ERROR("cannot draw into the given texture");
    }
    glFramebufferTexture2D(
        GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)prev_fbo);
    TRX_GL_Context_SwitchToViewport(TRX_GL_Context_GetViewport());
    TRX_GL_CheckError();
}

GLuint TRX_GL_Renderer_ResolveSceneFbo(void)
{
    M_CONTEXT *const p = (M_CONTEXT *)g_TRX_GL_Renderer.priv;
    GLint prev_fbo = 0;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &prev_fbo);
    const TRX_GL_FBO *const fbo = M_ResolveScene(p);
    glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)prev_fbo);
    return fbo->fbo;
}
