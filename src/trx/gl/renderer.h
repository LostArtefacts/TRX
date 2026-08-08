#pragma once

#include <trx/gl/config.h>

#include <GL/glew.h>

typedef struct TRX_GL_Renderer {
    void (*init)(struct TRX_GL_Renderer *renderer, const TRX_GL_CONFIG *config);
    void (*shutdown)(struct TRX_GL_Renderer *renderer);
    void (*swap_buffers)(struct TRX_GL_Renderer *renderer);
    void *priv;
} TRX_GL_RENDERER;

extern TRX_GL_RENDERER g_TRX_GL_Renderer;

// Bind the geometry framebuffer for rendering the 3D scene.
void TRX_GL_Renderer_BindGeometryFbo(void);

// Bind the UI framebuffer for rendering the UI overlay.
void TRX_GL_Renderer_BindUiFbo(void);

// Resolve the geometry framebuffer down to VIEWPORT_SCENE and return the GL
// object id holding the result (3D scene only, no UI). Multisampling and
// supersampling both need this before the scene can be sampled or read back;
// with neither of them on, the geometry framebuffer already is that result and
// is returned as is. Resolving twice in a frame costs nothing.
GLuint TRX_GL_Renderer_ResolveSceneFbo(void);
