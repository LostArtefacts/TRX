#pragma once

#include "gfx/3d/vertex_stream.h"
#include "gfx/gl/program.h"
#include "gfx/gl/sampler.h"
#include "gfx/gl/texture.h"

#define GFX_MAX_TEXTURES 128
#define GFX_NO_TEXTURE (-1)

#ifdef __cplusplus
    #include <cstdint>
extern "C" {
#else
    #include <stdint.h>
#endif

typedef struct GFX_3D_Renderer {
    bool wireframe;
    GFX_GL_Program program;
    GFX_GL_Sampler sampler;
    GFX_3D_VertexStream vertex_stream;

    GFX_GL_Texture *textures[GFX_MAX_TEXTURES];

    // state data
    C3D_HTX selected_texture_num;
    C3D_EASRC easrc;
    C3D_EADST eadst;

    // shader variable locations
    GLint loc_mat_projection;
    GLint loc_mat_model_view;
    GLint loc_tmap_en;
} GFX_3D_Renderer;

void GFX_3D_Renderer_Init(GFX_3D_Renderer *renderer);
void GFX_3D_Renderer_Close(GFX_3D_Renderer *renderer);

void GFX_3D_Renderer_RenderBegin(GFX_3D_Renderer *renderer);
void GFX_3D_Renderer_RenderEnd(GFX_3D_Renderer *renderer);

int GFX_3D_Renderer_TextureReg(
    GFX_3D_Renderer *renderer, const void *data, int width, int height);
bool GFX_3D_Renderer_TextureUnreg(GFX_3D_Renderer *renderer, int texture_num);

bool GFX_3D_Renderer_SetState(
    GFX_3D_Renderer *renderer, C3D_ERSID eRStateID, C3D_PRSDATA pRStateData);

void GFX_3D_Renderer_TmapEnable(GFX_3D_Renderer *renderer, bool is_enabled);
void GFX_3D_Renderer_TmapSelect(GFX_3D_Renderer *renderer, int texture_num);
void GFX_3D_Renderer_TmapFilter(
    GFX_3D_Renderer *renderer, C3D_ETEXFILTER filter);
void GFX_3D_Renderer_TmapRestore(GFX_3D_Renderer *renderer);

void GFX_3D_Renderer_RenderPrimStrip(
    GFX_3D_Renderer *renderer, C3D_VSTRIP strip, int count);
void GFX_3D_Renderer_RenderPrimList(
    GFX_3D_Renderer *renderer, C3D_VLIST list, int count);

void GFX_3D_Renderer_SetPrimType(GFX_3D_Renderer *renderer, C3D_EPRIM value);
void GFX_3D_Renderer_SetAlphaSrc(GFX_3D_Renderer *renderer, C3D_EASRC value);
void GFX_3D_Renderer_SetAlphaDst(GFX_3D_Renderer *renderer, C3D_EADST value);

#ifdef __cplusplus
}
#endif
