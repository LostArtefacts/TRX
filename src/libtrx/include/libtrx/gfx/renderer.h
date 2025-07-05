#pragma once

#include "config.h"

typedef struct GFX_Renderer {
    void (*init)(struct GFX_Renderer *renderer, const GFX_CONFIG *config);
    void (*shutdown)(struct GFX_Renderer *renderer);
    void (*swap_buffers)(struct GFX_Renderer *renderer);
    void *priv;
} GFX_RENDERER;

extern GFX_RENDERER g_GFX_Renderer;

// Bind the geometry framebuffer for rendering the 3D scene.
void GFX_Renderer_BindGeometryFbo(void);

// Bind the UI framebuffer for rendering the UI overlay.
void GFX_Renderer_BindUiFbo(void);
