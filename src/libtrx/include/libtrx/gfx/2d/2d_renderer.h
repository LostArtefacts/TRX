#pragma once

#include "../config.h"
#include "../gl/buffer.h"
#include "../gl/program.h"
#include "../gl/texture.h"
#include "../gl/vertex_array.h"
#include "2d_surface.h"

#include <stdint.h>

typedef struct {
    uint8_t r, g, b;
} GFX_COLOR;

typedef struct {
    float x0;
    float y0;
    float x1;
    float y1;
} GFX_TEXTURE_SIZE;

typedef enum {
    GFX_2D_EFFECT_NONE = 0,
    GFX_2D_EFFECT_VIGNETTE = 1 << 0,
    GFX_2D_EFFECT_WAVE = 1 << 1,
} GFX_2D_EFFECT;

typedef struct GFX_2D_RENDERER GFX_2D_RENDERER;

GFX_2D_RENDERER *GFX_2D_Renderer_Create(void);
void GFX_2D_Renderer_Destroy(GFX_2D_RENDERER *renderer);

void GFX_2D_Renderer_Upload(
    GFX_2D_RENDERER *renderer, GFX_2D_SURFACE_DESC *desc, const uint8_t *data);

// Set the normalized screen-space quad for rendering.
// Coordinates are in [0,1] where (0,0) is top-left and (1,1) is bottom-right.
void GFX_2D_Renderer_SetQuad(
    GFX_2D_RENDERER *renderer, float x0, float y0, float x1, float y1);

void GFX_2D_Renderer_SetRepeat(GFX_2D_RENDERER *renderer, int32_t x, int32_t y);
void GFX_2D_Renderer_SetTextureSize(
    GFX_2D_RENDERER *renderer, const GFX_TEXTURE_SIZE *size);
void GFX_2D_Renderer_SetEffect(GFX_2D_RENDERER *renderer, uint32_t effect);

void GFX_2D_Renderer_Render(GFX_2D_RENDERER *renderer);
