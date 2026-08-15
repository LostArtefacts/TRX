#pragma once

#include <trx/core/result.h>
#include <trx/gl/config.h>
#include <trx/gl/texture.h>

#include <GL/glew.h>
#include <stdint.h>

typedef struct TRX_GL_Renderer {
    RESULT (*init)(
        struct TRX_GL_Renderer *renderer, const TRX_GL_CONFIG *config);
    void (*shutdown)(struct TRX_GL_Renderer *renderer);
    void (*swap_buffers)(struct TRX_GL_Renderer *renderer);
    void *priv;
} TRX_GL_RENDERER;

extern TRX_GL_RENDERER g_TRX_GL_Renderer;

// Bind the geometry framebuffer for rendering the 3D scene.
void TRX_GL_Renderer_BindGeometryFbo(void);

// Bind the UI framebuffer for rendering the UI overlay.
void TRX_GL_Renderer_BindUiFbo(void);

// Resize the framebuffers to the current viewport sizes. The renderer does
// this itself once the frame has been presented; call it when the viewports
// change between frames, so the next frame is not drawn into framebuffers of
// the old size.
void TRX_GL_Renderer_SyncFboSizes(void);

// Resolve the geometry framebuffer down to VIEWPORT_SCENE and return the GL
// object id holding the result (3D scene only, no UI). Multisampling and
// supersampling both need this before the scene can be sampled or read back;
// with neither of them on, the geometry framebuffer already is that result and
// is returned as is. Resolving twice in a frame costs nothing.
GLuint TRX_GL_Renderer_ResolveSceneFbo(void);

// Draw the scene and the UI over it into the given texture, the same way the
// frame is put together for the window. Called between frames, before anything
// has drawn over the framebuffers, this reproduces the frame the player is
// looking at; the texture is written in full, at its own size.
void TRX_GL_Renderer_CompositeToTexture(
    const TRX_GL_TEXTURE *texture, int32_t width, int32_t height);
