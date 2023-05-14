#pragma once

#include "gfx/gl/buffer.h"
#include "gfx/gl/gl_core_3_3.h"
#include "gfx/gl/program.h"
#include "gfx/gl/sampler.h"
#include "gfx/gl/texture.h"
#include "gfx/gl/vertex_array.h"

#include <stdbool.h>
#include <stdint.h>

// TODO: make customizable
// TODO: triple check the behavior for different aspect ratios
#define FBO_WIDTH 800
#define FBO_HEIGHT 600

typedef struct GFX_FBO_Renderer {
    GLuint fbo;
    GLuint rbo;

    GFX_GL_VertexArray vertex_array;
    GFX_GL_Buffer buffer;
    GFX_GL_Texture texture;
    GFX_GL_Sampler sampler;
    GFX_GL_Program program;
} GFX_FBO_Renderer;

void GFX_FBO_Renderer_Init(GFX_FBO_Renderer *renderer);
void GFX_FBO_Renderer_Close(GFX_FBO_Renderer *renderer);
void GFX_FBO_Renderer_Render(GFX_FBO_Renderer *renderer);
void GFX_FBO_Renderer_SetSmoothingEnabled(
    GFX_FBO_Renderer *renderer, bool is_enabled);
