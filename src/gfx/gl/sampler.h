#pragma once

#include "gfx/gl/gl_core_3_3.h"

typedef struct GFX_GL_Sampler {
    GLuint id;
} GFX_GL_Sampler;

void GFX_GL_Sampler_Init(GFX_GL_Sampler *sampler);
void GFX_GL_Sampler_Close(GFX_GL_Sampler *sampler);

void GFX_GL_Sampler_Bind(GFX_GL_Sampler *sampler, GLuint unit);
void GFX_GL_Sampler_Parameteri(
    GFX_GL_Sampler *sampler, GLenum pname, GLint param);
void GFX_GL_Sampler_Parameterf(
    GFX_GL_Sampler *sampler, GLenum pname, GLfloat param);
