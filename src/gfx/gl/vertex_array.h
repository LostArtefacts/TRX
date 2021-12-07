#pragma once

#include "gfx/gl/gl_core_3_3.h"

typedef struct GFX_GL_VertexArray {
    GLuint id;
} GFX_GL_VertexArray;

void GFX_GL_VertexArray_Init(GFX_GL_VertexArray *array);
void GFX_GL_VertexArray_Close(GFX_GL_VertexArray *array);
void GFX_GL_VertexArray_Bind(GFX_GL_VertexArray *array);
void GFX_GL_VertexArray_Attribute(
    GFX_GL_VertexArray *array, GLuint index, GLint size, GLenum type,
    GLboolean normalized, GLsizei stride, GLsizei offset);
