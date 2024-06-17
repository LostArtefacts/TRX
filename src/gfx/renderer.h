#pragma once

typedef struct GFX_Renderer {
    void (*init)(struct GFX_Renderer *renderer);
    void (*shutdown)(struct GFX_Renderer *renderer);
    void (*reset)(struct GFX_Renderer *renderer);
    void (*swap_buffers)(struct GFX_Renderer *renderer);
    void *priv;
} GFX_Renderer;
