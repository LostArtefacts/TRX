#pragma once

#include "../common.h"
#include "../config.h"
#include "../gl/gl_core_3_3.h"
#include "../gl/program.h"
#include "../gl/sampler.h"
#include "../gl/texture.h"
#include "vertex_stream.h"

#define GFX_MAX_TEXTURES 128
#define GFX_NO_TEXTURE (-1)
#define GFX_ENV_MAP_TEXTURE (-2)

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    GFX_BLEND_MODE_OFF,
    GFX_BLEND_MODE_NORMAL,
    GFX_BLEND_MODE_MULTIPLY,
} GFX_BLEND_MODE;

typedef struct {
    const GFX_CONFIG *config;

    GFX_GL_PROGRAM program;
    GFX_GL_SAMPLER sampler;
    GFX_3D_VERTEX_STREAM vertex_stream;

    GFX_GL_TEXTURE *textures[GFX_MAX_TEXTURES];
    GFX_GL_TEXTURE *env_map_texture;
    int selected_texture_num;

    // shader variable locations
    GLint loc_mat_projection;
    GLint loc_mat_model_view;
    GLint loc_texturing_enabled;
    GLint loc_smoothing_enabled;
} GFX_3D_RENDERER;

void GFX_3D_Renderer_Init(GFX_3D_RENDERER *renderer, const GFX_CONFIG *config);
void GFX_3D_Renderer_Close(GFX_3D_RENDERER *renderer);

void GFX_3D_Renderer_RenderBegin(GFX_3D_RENDERER *renderer);
void GFX_3D_Renderer_RenderEnd(GFX_3D_RENDERER *renderer);
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
void GFX_3D_Renderer_RestoreTexture(GFX_3D_RENDERER *renderer);

void GFX_3D_Renderer_RenderPrimStrip(
    GFX_3D_RENDERER *renderer, GFX_3D_VERTEX *vertices, int count);
void GFX_3D_Renderer_RenderPrimFan(
    GFX_3D_RENDERER *renderer, GFX_3D_VERTEX *vertices, int count);
void GFX_3D_Renderer_RenderPrimList(
    GFX_3D_RENDERER *renderer, GFX_3D_VERTEX *vertices, int count);

void GFX_3D_Renderer_SetPrimType(
    GFX_3D_RENDERER *renderer, GFX_3D_PRIM_TYPE value);
void GFX_3D_Renderer_SetTextureFilter(
    GFX_3D_RENDERER *renderer, GFX_TEXTURE_FILTER filter);
void GFX_3D_Renderer_SetDepthTestEnabled(
    GFX_3D_RENDERER *renderer, bool is_enabled);
void GFX_3D_Renderer_SetBlendingMode(
    GFX_3D_RENDERER *renderer, GFX_BLEND_MODE blend_mode);
void GFX_3D_Renderer_SetTexturingEnabled(
    GFX_3D_RENDERER *renderer, bool is_enabled);
