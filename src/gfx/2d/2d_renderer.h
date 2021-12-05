#pragma once

#include "gfx/gl/buffer.h"
#include "gfx/gl/program.h"
#include "gfx/gl/sampler.h"
#include "gfx/gl/texture.h"
#include "gfx/gl/vertex_array.h"

#ifdef __cplusplus
    #include <cstdint>
extern "C" {
#else
    #include <stdint.h>
#endif

typedef struct GFX_2D_SurfaceDesc {
    int width;
    int height;
    int pitch;
    bool has_back_buffer;
    void *pixels;
    int bit_count;
    struct {
        bool front;
        bool primary;
        bool flip;
    } flags;
} GFX_2D_SurfaceDesc;

typedef struct GFX_2D_Renderer {
    uint32_t width;
    uint32_t height;
    GFX_GL_VertexArray surface_format;
    GFX_GL_Buffer surface_buffer;
    GFX_GL_Texture surface_texture;
    GFX_GL_Sampler sampler;
    GFX_GL_Program program;
} GFX_2D_Renderer;

void GFX_2D_Renderer_Init(GFX_2D_Renderer *renderer);
void GFX_2D_Renderer_Close(GFX_2D_Renderer *renderer);

void GFX_2D_Renderer_Upload(
    GFX_2D_Renderer *renderer, GFX_2D_SurfaceDesc *desc, const uint8_t *data);
void GFX_2D_Renderer_Render(GFX_2D_Renderer *renderer);

#ifdef __cplusplus
}
#endif
