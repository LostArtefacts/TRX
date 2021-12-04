#pragma once

#include "glrage_gl/gl_core_3_3.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct GLRage_GLVertexArray {
    GLuint id;
} GLRage_GLVertexArray;

void GLRage_GLVertexArray_Init(GLRage_GLVertexArray *array);
void GLRage_GLVertexArray_Close(GLRage_GLVertexArray *array);
void GLRage_GLVertexArray_Bind(GLRage_GLVertexArray *array);
void GLRage_GLVertexArray_Attribute(
    GLRage_GLVertexArray *array, GLuint index, GLint size, GLenum type,
    GLboolean normalized, GLsizei stride, GLsizei offset);

#ifdef __cplusplus
}
#endif
