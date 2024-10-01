#pragma once

#include "../gl/gl_core_3_3.h"

typedef struct {
    GLuint id;
} GFX_GL_SAMPLER;

void GFX_GL_Sampler_Init(GFX_GL_SAMPLER *sampler);
void GFX_GL_Sampler_Close(GFX_GL_SAMPLER *sampler);

void GFX_GL_Sampler_Bind(GFX_GL_SAMPLER *sampler, GLuint unit);
void GFX_GL_Sampler_Parameteri(
    GFX_GL_SAMPLER *sampler, GLenum pname, GLint param);
void GFX_GL_Sampler_Parameterf(
    GFX_GL_SAMPLER *sampler, GLenum pname, GLfloat param);
