#pragma once

#include "gfx/3d/vertex_stream.h"
#include "gfx/common.h"
#include "gfx/config.h"
#include "gfx/gl/gl_core_3_3.h"
#include "gfx/gl/program.h"
#include "gfx/gl/sampler.h"
#include "gfx/gl/texture.h"

#define GFX_MAX_TEXTURES 128
#define GFX_NO_TEXTURE (-1)
#define GFX_ENV_MAP_TEXTURE (-2)

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    GFX_BlendMode_Off,
    GFX_BlendMode_Normal,
    GFX_BlendMode_Multiply,
} GFX_BlendMode;

typedef struct GFX_3D_Renderer {
    const GFX_CONFIG *config;

    GFX_GL_Program program;
    GFX_GL_Sampler sampler;
    GFX_3D_VertexStream vertex_stream;

    GFX_GL_Texture *textures[GFX_MAX_TEXTURES];
    GFX_GL_Texture *env_map_texture;
    int selected_texture_num;

    // shader variable locations
    GLint loc_mat_projection;
    GLint loc_mat_model_view;
    GLint loc_texturing_enabled;
    GLint loc_smoothing_enabled;
} GFX_3D_Renderer;

void GFX_3D_Renderer_Init(GFX_3D_Renderer *renderer, const GFX_CONFIG *config);
void GFX_3D_Renderer_Close(GFX_3D_Renderer *renderer);

void GFX_3D_Renderer_RenderBegin(GFX_3D_Renderer *renderer);
void GFX_3D_Renderer_RenderEnd(GFX_3D_Renderer *renderer);
void GFX_3D_Renderer_ClearDepth(GFX_3D_Renderer *renderer);

int GFX_3D_Renderer_RegisterTexturePage(
    GFX_3D_Renderer *renderer, const void *data, int width, int height);
bool GFX_3D_Renderer_UnregisterTexturePage(
    GFX_3D_Renderer *renderer, int texture_num);

int GFX_3D_Renderer_RegisterEnvironmentMap(GFX_3D_Renderer *renderer);
bool GFX_3D_Renderer_UnregisterEnvironmentMap(
    GFX_3D_Renderer *renderer, int texture_num);
void GFX_3D_Renderer_FillEnvironmentMap(GFX_3D_Renderer *renderer);

void GFX_3D_Renderer_SelectTexture(GFX_3D_Renderer *renderer, int texture_num);
void GFX_3D_Renderer_RestoreTexture(GFX_3D_Renderer *renderer);

void GFX_3D_Renderer_RenderPrimStrip(
    GFX_3D_Renderer *renderer, GFX_3D_Vertex *vertices, int count);
void GFX_3D_Renderer_RenderPrimFan(
    GFX_3D_Renderer *renderer, GFX_3D_Vertex *vertices, int count);
void GFX_3D_Renderer_RenderPrimList(
    GFX_3D_Renderer *renderer, GFX_3D_Vertex *vertices, int count);

void GFX_3D_Renderer_SetPrimType(
    GFX_3D_Renderer *renderer, GFX_3D_PrimType value);
void GFX_3D_Renderer_SetTextureFilter(
    GFX_3D_Renderer *renderer, GFX_TEXTURE_FILTER filter);
void GFX_3D_Renderer_SetDepthTestEnabled(
    GFX_3D_Renderer *renderer, bool is_enabled);
void GFX_3D_Renderer_SetBlendingMode(
    GFX_3D_Renderer *renderer, GFX_BlendMode blend_mode);
void GFX_3D_Renderer_SetTexturingEnabled(
    GFX_3D_Renderer *renderer, bool is_enabled);
