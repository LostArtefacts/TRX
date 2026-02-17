#pragma once

#include <trx/gl/2d/2d_surface.h>
#include <trx/gl/config.h>
#include <trx/gl/gl/buffer.h>
#include <trx/gl/gl/program.h>
#include <trx/gl/gl/texture.h>
#include <trx/gl/gl/vertex_array.h>

#include <stdint.h>

typedef struct {
    float x0;
    float y0;
    float x1;
    float y1;
} TRX_GL_TEXTURE_SIZE;

typedef enum {
    TRX_GL_2D_EFFECT_NONE = 0,
    TRX_GL_2D_EFFECT_VIGNETTE = 1 << 0,
    TRX_GL_2D_EFFECT_WAVE = 1 << 1,
} TRX_GL_2D_EFFECT;

typedef struct TRX_GL_2D_RENDERER TRX_GL_2D_RENDERER;

typedef enum {
    TRX_GL_2D_FIT_STRETCH,
    TRX_GL_2D_FIT_LETTERBOX,
    TRX_GL_2D_FIT_CROP,
    TRX_GL_2D_FIT_SMART,
} TRX_GL_2D_FIT_MODE;

TRX_GL_2D_RENDERER *TRX_GL_2D_Renderer_Create(void);
void TRX_GL_2D_Renderer_Destroy(TRX_GL_2D_RENDERER *renderer);

void TRX_GL_2D_Renderer_Upload(
    TRX_GL_2D_RENDERER *renderer, TRX_GL_2D_SURFACE_DESC *desc,
    const uint8_t *data);

// Use an external GL texture instead of the internally managed one.
// When enabled, the renderer uses a full [0,1] UV mapping by default.
void TRX_GL_2D_Renderer_SetExternalTexture(
    TRX_GL_2D_RENDERER *renderer, GLuint texture_id, int32_t width,
    int32_t height, bool flip_y);
void TRX_GL_2D_Renderer_ClearExternalTexture(TRX_GL_2D_RENDERER *renderer);

void TRX_GL_2D_Renderer_SetRepeat(
    TRX_GL_2D_RENDERER *renderer, int32_t x, int32_t y);
void TRX_GL_2D_Renderer_SetTextureSize(
    TRX_GL_2D_RENDERER *renderer, const TRX_GL_TEXTURE_SIZE *size);
void TRX_GL_2D_Renderer_SetEffect(
    TRX_GL_2D_RENDERER *renderer, uint32_t effect);

void TRX_GL_2D_Renderer_SetOpacity(TRX_GL_2D_RENDERER *renderer, float opacity);
void TRX_GL_2D_Renderer_SetBrightnessScale(
    TRX_GL_2D_RENDERER *renderer, float brightness_scale);

void TRX_GL_2D_Renderer_SetFit(
    TRX_GL_2D_RENDERER *renderer, TRX_GL_2D_FIT_MODE fit_mode, float src_w,
    float src_h);
void TRX_GL_2D_Renderer_ClearFit(TRX_GL_2D_RENDERER *renderer);

void TRX_GL_2D_Renderer_Render(TRX_GL_2D_RENDERER *renderer);
void TRX_GL_2D_Renderer_RenderWithBlend(TRX_GL_2D_RENDERER *renderer);
