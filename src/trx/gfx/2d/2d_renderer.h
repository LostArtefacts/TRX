#pragma once

#include <trx/gfx/2d/2d_surface.h>
#include <trx/gfx/config.h>
#include <trx/gfx/gl/buffer.h>
#include <trx/gfx/gl/program.h>
#include <trx/gfx/gl/texture.h>
#include <trx/gfx/gl/vertex_array.h>

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

// Use an external GL texture instead of the internally managed one.
// When enabled, the renderer uses a full [0,1] UV mapping by default.
void GFX_2D_Renderer_SetExternalTexture(
    GFX_2D_RENDERER *renderer, GLuint texture_id, int32_t width, int32_t height,
    bool flip_y);
void GFX_2D_Renderer_ClearExternalTexture(GFX_2D_RENDERER *renderer);

// Set the normalized screen-space quad for rendering.
// Coordinates are in [0,1] where (0,0) is top-left and (1,1) is bottom-right.
void GFX_2D_Renderer_SetQuad(
    GFX_2D_RENDERER *renderer, float x0, float y0, float x1, float y1);

void GFX_2D_Renderer_SetRepeat(GFX_2D_RENDERER *renderer, int32_t x, int32_t y);
void GFX_2D_Renderer_SetTextureSize(
    GFX_2D_RENDERER *renderer, const GFX_TEXTURE_SIZE *size);
void GFX_2D_Renderer_SetEffect(GFX_2D_RENDERER *renderer, uint32_t effect);

void GFX_2D_Renderer_SetOpacity(GFX_2D_RENDERER *renderer, float opacity);

void GFX_2D_Renderer_Render(GFX_2D_RENDERER *renderer);
void GFX_2D_Renderer_RenderWithBlend(GFX_2D_RENDERER *renderer);
