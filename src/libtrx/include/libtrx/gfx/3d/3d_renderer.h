#pragma once

#include "../common.h"
#include "../config.h"
#include "../gl/program.h"
#include "../gl/sampler.h"
#include "../gl/texture.h"
#include "vertex_stream.h"

#include <GL/glew.h>

#define GFX_MAX_TEXTURES 128
#define GFX_NO_TEXTURE (-1)
#define GFX_ENV_MAP_TEXTURE (-2)

#include <stdint.h>

typedef enum {
    GFX_BLEND_MODE_OFF,
    GFX_BLEND_MODE_NORMAL,
    GFX_BLEND_MODE_MULTIPLY,
} GFX_BLEND_MODE;

typedef struct GFX_3D_RENDERER GFX_3D_RENDERER;

GFX_3D_RENDERER *GFX_3D_Renderer_Create(void);
void GFX_3D_Renderer_Destroy(GFX_3D_RENDERER *renderer);

void GFX_3D_Renderer_RenderBegin(GFX_3D_RENDERER *renderer);
void GFX_3D_Renderer_RenderEnd(GFX_3D_RENDERER *renderer);
void GFX_3D_Renderer_Flush(GFX_3D_RENDERER *renderer);
void GFX_3D_Renderer_ClearDepth(GFX_3D_RENDERER *renderer);

int GFX_3D_Renderer_RegisterTexturePage(
    GFX_3D_RENDERER *renderer, const void *data, int width, int height);
bool GFX_3D_Renderer_UnregisterTexturePage(
    GFX_3D_RENDERER *renderer, int texture_num);

int GFX_3D_Renderer_RegisterEnvironmentMap(GFX_3D_RENDERER *renderer);
bool GFX_3D_Renderer_UnregisterEnvironmentMap(
    GFX_3D_RENDERER *renderer, int texture_num);
void GFX_3D_Renderer_FillEnvironmentMap(GFX_3D_RENDERER *renderer);

void GFX_3D_Renderer_SelectTexture(GFX_3D_RENDERER *renderer, int texture_num);

void GFX_3D_Renderer_RenderPrimStrip(
    GFX_3D_RENDERER *renderer, const GFX_3D_VERTEX *vertices, int count);
void GFX_3D_Renderer_RenderPrimFan(
    GFX_3D_RENDERER *renderer, const GFX_3D_VERTEX *vertices, int count);
void GFX_3D_Renderer_RenderPrimList(
    GFX_3D_RENDERER *renderer, const GFX_3D_VERTEX *vertices, int count);

void GFX_3D_Renderer_SetPrimType(
    GFX_3D_RENDERER *renderer, GFX_3D_PRIM_TYPE value);
void GFX_3D_Renderer_SetTextureFilter(
    GFX_3D_RENDERER *renderer, GFX_TEXTURE_FILTER filter);
void GFX_3D_Renderer_SetDepthWritesEnabled(
    GFX_3D_RENDERER *renderer, bool is_enabled);
void GFX_3D_Renderer_SetDepthTestEnabled(
    GFX_3D_RENDERER *renderer, bool is_enabled);
void GFX_3D_Renderer_SetDepthBufferEnabled(
    GFX_3D_RENDERER *renderer, bool is_enabled);
void GFX_3D_Renderer_SetBlendingMode(
    GFX_3D_RENDERER *renderer, GFX_BLEND_MODE blend_mode);
void GFX_3D_Renderer_SetTexturingEnabled(
    GFX_3D_RENDERER *renderer, bool is_enabled);
void GFX_3D_Renderer_SetAnisotropyFilter(
    GFX_3D_RENDERER *renderer, float value);
void GFX_3D_Renderer_SetAlphaPointDiscard(
    GFX_3D_RENDERER *renderer, bool is_enabled);
void GFX_3D_Renderer_SetAlphaThreshold(GFX_3D_RENDERER *renderer, float value);
void GFX_3D_Renderer_SetBrightnessMultiplier(
    GFX_3D_RENDERER *renderer, float value);
