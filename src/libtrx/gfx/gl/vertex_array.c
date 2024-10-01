#include "gfx/gl/vertex_array.h"

#include "gfx/gl/utils.h"

#include <assert.h>
#include <stdint.h>

void GFX_GL_VertexArray_Init(GFX_GL_VERTEX_ARRAY *array)
{
    assert(array);
    glGenVertexArrays(1, &array->id);
    GFX_GL_CheckError();
}

void GFX_GL_VertexArray_Close(GFX_GL_VERTEX_ARRAY *array)
{
    assert(array);
    glDeleteVertexArrays(1, &array->id);
    GFX_GL_CheckError();
}

void GFX_GL_VertexArray_Bind(GFX_GL_VERTEX_ARRAY *array)
{
    assert(array);
    glBindVertexArray(array->id);
    GFX_GL_CheckError();
}

void GFX_GL_VertexArray_Attribute(
    GFX_GL_VERTEX_ARRAY *array, GLuint index, GLint size, GLenum type,
    GLboolean normalized, GLsizei stride, GLsizei offset)
{
    assert(array);
    glEnableVertexAttribArray(index);
    GFX_GL_CheckError();

    glVertexAttribPointer(
        index, size, type, normalized, stride, (void *)(intptr_t)offset);
    GFX_GL_CheckError();
}
