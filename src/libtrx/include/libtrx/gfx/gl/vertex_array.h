#pragma once

#include <GL/glew.h>

typedef struct {
    bool initialized;
    GLuint id;
} GFX_GL_VERTEX_ARRAY;

void GFX_GL_VertexArray_Init(GFX_GL_VERTEX_ARRAY *array);
void GFX_GL_VertexArray_Close(GFX_GL_VERTEX_ARRAY *array);
void GFX_GL_VertexArray_Bind(GFX_GL_VERTEX_ARRAY *array);
void GFX_GL_VertexArray_Attribute(
    GFX_GL_VERTEX_ARRAY *array, GLuint index, GLint size, GLenum type,
    GLboolean normalized, GLsizei stride, GLsizei offset);
