#include "glrage_gl/vertex_array.h"

#include <assert.h>

void GLRage_GLVertexArray_Init(GLRage_GLVertexArray *array)
{
    assert(array);
    glGenVertexArrays(1, &array->id);
}

void GLRage_GLVertexArray_Close(GLRage_GLVertexArray *array)
{
    assert(array);
    glDeleteVertexArrays(1, &array->id);
}

void GLRage_GLVertexArray_Bind(GLRage_GLVertexArray *array)
{
    assert(array);
    glBindVertexArray(array->id);
}

void GLRage_GLVertexArray_Attribute(
    GLRage_GLVertexArray *array, GLuint index, GLint size, GLenum type,
    GLboolean normalized, GLsizei stride, GLsizei offset)
{
    assert(array);
    glEnableVertexAttribArray(index);
    glVertexAttribPointer(
        index, size, type, normalized, stride, (void *)offset);
}
