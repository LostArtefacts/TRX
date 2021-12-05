#include "gfx/gl/vertex_array.h"

#include <assert.h>

void GFX_GL_VertexArray_Init(GFX_GL_VertexArray *array)
{
    assert(array);
    glGenVertexArrays(1, &array->id);
}

void GFX_GL_VertexArray_Close(GFX_GL_VertexArray *array)
{
    assert(array);
    glDeleteVertexArrays(1, &array->id);
}

void GFX_GL_VertexArray_Bind(GFX_GL_VertexArray *array)
{
    assert(array);
    glBindVertexArray(array->id);
}

void GFX_GL_VertexArray_Attribute(
    GFX_GL_VertexArray *array, GLuint index, GLint size, GLenum type,
    GLboolean normalized, GLsizei stride, GLsizei offset)
{
    assert(array);
    glEnableVertexAttribArray(index);
    glVertexAttribPointer(
        index, size, type, normalized, stride, (void *)offset);
}
