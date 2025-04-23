#pragma once

#include "config.h"

typedef struct GFX_Renderer {
    void (*init)(struct GFX_Renderer *renderer, const GFX_CONFIG *config);
    void (*shutdown)(struct GFX_Renderer *renderer);
    void (*reset)(struct GFX_Renderer *renderer);
    void (*swap_buffers)(struct GFX_Renderer *renderer);
    void *priv;
} GFX_RENDERER;
