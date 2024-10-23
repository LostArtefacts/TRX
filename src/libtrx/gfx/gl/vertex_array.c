#include "gfx/gl/vertex_array.h"

#include "gfx/gl/utils.h"

#include <assert.h>
#include <stdint.h>

void GFX_GL_VertexArray_Init(GFX_GL_VERTEX_ARRAY *array)
{
    assert(array != NULL);
    glGenVertexArrays(1, &array->id);
    GFX_GL_CheckError();
    array->initialized = true;
}

void GFX_GL_VertexArray_Close(GFX_GL_VERTEX_ARRAY *array)
{
    assert(array != NULL);
    if (array->initialized) {
        glDeleteVertexArrays(1, &array->id);
        GFX_GL_CheckError();
    }
    array->initialized = false;
}

void GFX_GL_VertexArray_Bind(GFX_GL_VERTEX_ARRAY *array)
{
    assert(array != NULL);
    assert(array->initialized);
    glBindVertexArray(array->id);
    GFX_GL_CheckError();
}

void GFX_GL_VertexArray_Attribute(
    GFX_GL_VERTEX_ARRAY *array, GLuint index, GLint size, GLenum type,
    GLboolean normalized, GLsizei stride, GLsizei offset)
{
    assert(array != NULL);
    assert(array->initialized);
    glEnableVertexAttribArray(index);
    GFX_GL_CheckError();

    glVertexAttribPointer(
        index, size, type, normalized, stride, (void *)(intptr_t)offset);
    GFX_GL_CheckError();
}
