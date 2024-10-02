#pragma once

#include "../gl/buffer.h"
#include "../gl/program.h"
#include "../gl/sampler.h"
#include "../gl/texture.h"
#include "../gl/vertex_array.h"
#include "2d_surface.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint32_t width;
    uint32_t height;
    GFX_GL_VERTEX_ARRAY surface_format;
    GFX_GL_BUFFER surface_buffer;
    GFX_GL_TEXTURE surface_texture;
    GFX_GL_SAMPLER sampler;
    GFX_GL_PROGRAM program;
} GFX_2D_RENDERER;

void GFX_2D_Renderer_Init(GFX_2D_RENDERER *renderer);
void GFX_2D_Renderer_Close(GFX_2D_RENDERER *renderer);

void GFX_2D_Renderer_Upload(
    GFX_2D_RENDERER *renderer, GFX_2D_SURFACE_DESC *desc, const uint8_t *data);
void GFX_2D_Renderer_Render(GFX_2D_RENDERER *renderer);
